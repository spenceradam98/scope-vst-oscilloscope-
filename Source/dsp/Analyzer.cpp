#include "Analyzer.h"

namespace scope
{
static float toDb (float lin) noexcept { return lin > 1.0e-6f ? 20.0f * std::log10 (lin) : -120.0f; }

Analyzer::Analyzer()
    : histL ((size_t) historySize, 0.0f), histR ((size_t) historySize, 0.0f),
      fftData ((size_t) fftSize * 2, 0.0f), specDb ((size_t) numBins, -120.0f), holdDb ((size_t) numBins, -120.0f)
{
    setSampleRate (48000.0);
}

void Analyzer::setSampleRate (double sr)
{
    if (sr <= 0.0)
        return;
    sampleRate = sr;
    rmsCoef = 1.0f - std::exp (-1.0f / (0.3f * (float) sr));   // 300 ms RMS
    corrCoef = 1.0f - std::exp (-1.0f / (0.25f * (float) sr)); // 250 ms correlation
    blockLength = juce::jmax (1, (int) std::round (sr * 0.1)); // 100 ms loudness blocks
    designKWeighting();
    reset();
}

void Analyzer::reset()
{
    std::fill (histL.begin(), histL.end(), 0.0f);
    std::fill (histR.begin(), histR.end(), 0.0f);
    std::fill (specDb.begin(), specDb.end(), -120.0f);
    std::fill (holdDb.begin(), holdDb.end(), -120.0f);
    msL = msR = avgLR = avgLL = avgRR = 0.0f;
    for (auto& b : shelf) b.z1 = b.z2 = 0;
    for (auto& b : highpass) b.z1 = b.z2 = 0;
    blockSum = 0; blockCount = 0; blocks.fill (0.0); blockIndex = 0; blocksFilled = 0;
    lufsMomentary = lufsShortTerm = -120.0f;
    resetPeaks();
}

void Analyzer::resetPeaks()
{
    for (int c = 0; c < 2; ++c) { peak[c] = peakHold[c] = 0.0f; clipped[c] = false; peakHoldTimer[c] = 0.0f; }
    lufsMax = -120.0f;
}

// ITU-R BS.1770 K-weighting, coefficients computed for any sample rate.
void Analyzer::designKWeighting()
{
    const double fs = sampleRate;
    {
        const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
        const double K = std::tan (juce::MathConstants<double>::pi * f0 / fs);
        const double Vh = std::pow (10.0, G / 20.0);
        const double Vb = std::pow (Vh, 0.4996667741545416);
        const double a0 = 1.0 + K / Q + K * K;
        for (auto& s : shelf)
        {
            s.b0 = (float) ((Vh + Vb * K / Q + K * K) / a0);
            s.b1 = (float) (2.0 * (K * K - Vh) / a0);
            s.b2 = (float) ((Vh - Vb * K / Q + K * K) / a0);
            s.a1 = (float) (2.0 * (K * K - 1.0) / a0);
            s.a2 = (float) ((1.0 - K / Q + K * K) / a0);
        }
    }
    {
        const double f0 = 38.13547087602444, Q = 0.5003270373238773;
        const double K = std::tan (juce::MathConstants<double>::pi * f0 / fs);
        const double a0 = 1.0 + K / Q + K * K;
        for (auto& s : highpass)
        {
            s.b0 = 1.0f; s.b1 = -2.0f; s.b2 = 1.0f;
            s.a1 = (float) (2.0 * (K * K - 1.0) / a0);
            s.a2 = (float) ((1.0 - K / Q + K * K) / a0);
        }
    }
}

void Analyzer::push (const float* l, const float* r, int n)
{
    for (int i = 0; i < n; ++i)
    {
        const float L = l[i], R = r[i];
        histL[(size_t) writePos] = L;
        histR[(size_t) writePos] = R;
        writePos = (writePos + 1) & (historySize - 1);

        const float aL = std::abs (L), aR = std::abs (R);
        if (aL > peak[0]) peak[0] = aL;
        if (aR > peak[1]) peak[1] = aR;
        if (aL > peakHold[0]) { peakHold[0] = aL; peakHoldTimer[0] = 1.5f; }
        if (aR > peakHold[1]) { peakHold[1] = aR; peakHoldTimer[1] = 1.5f; }
        if (aL >= 1.0f) clipped[0] = true;
        if (aR >= 1.0f) clipped[1] = true;

        msL += rmsCoef * (L * L - msL);
        msR += rmsCoef * (R * R - msR);
        avgLR += corrCoef * (L * R - avgLR);
        avgLL += corrCoef * (L * L - avgLL);
        avgRR += corrCoef * (R * R - avgRR);

        const float kL = highpass[0].process (shelf[0].process (L));
        const float kR = highpass[1].process (shelf[1].process (R));
        blockSum += (double) kL * kL + (double) kR * kR;
        if (++blockCount >= blockLength)
            finishLoudnessBlock();
    }

    rms[0] = std::sqrt (juce::jmax (0.0f, msL));
    rms[1] = std::sqrt (juce::jmax (0.0f, msR));
    const float denom = std::sqrt (avgLL * avgRR);
    correlation = denom > 1.0e-9f ? juce::jlimit (-1.0f, 1.0f, avgLR / denom) : 0.0f;
}

void Analyzer::finishLoudnessBlock()
{
    blocks[(size_t) blockIndex] = blockSum / (double) blockCount;
    blockIndex = (blockIndex + 1) % (int) blocks.size();
    blocksFilled = juce::jmin (blocksFilled + 1, (int) blocks.size());
    blockSum = 0.0; blockCount = 0;

    auto meanOfLast = [this] (int count)
    {
        count = juce::jmin (count, blocksFilled);
        if (count == 0) return 0.0;
        double s = 0.0;
        for (int i = 1; i <= count; ++i)
            s += blocks[(size_t) ((blockIndex - i + (int) blocks.size()) % (int) blocks.size())];
        return s / count;
    };
    auto toLufs = [] (double ms) { return ms > 1.0e-12 ? (float) (-0.691 + 10.0 * std::log10 (ms)) : -120.0f; };

    lufsMomentary = toLufs (meanOfLast (4));
    lufsShortTerm = toLufs (meanOfLast (30));
    lufsMax = juce::jmax (lufsMax, lufsMomentary);
}

void Analyzer::decayMeters (float seconds)
{
    const float fall = std::pow (10.0f, -20.0f * seconds / 20.0f); // 20 dB/s ballistic fall
    const float vuCoef = 1.0f - std::exp (-seconds / 0.3f);
    for (int c = 0; c < 2; ++c)
    {
        vuLevel[c] += vuCoef * (rms[c] - vuLevel[c]);
        peak[c] *= fall;
        peakHoldTimer[c] -= seconds;
        if (peakHoldTimer[c] <= 0.0f)
            peakHold[c] *= std::pow (10.0f, -10.0f * seconds / 20.0f);
    }
    // Holds fall slowly in the spectrum too
    const float holdFall = 6.0f * seconds;
    for (auto& h : holdDb) h -= holdFall;
}

void Analyzer::updateSpectrum()
{
    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = mid (fftSize - 1 - i);
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);

    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

    const float scale = 4.0f / (float) fftSize; // Hann coherent gain 0.5, one-sided x2
    const float release = releaseDbPerSecond / 60.0f;

    int best = -1; float bestDb = -200.0f;
    const int minBin = juce::jmax (1, (int) (20.0f / binToFreq (1.0f)));
    const int maxBin = juce::jmin (numBins - 2, (int) (20000.0f / binToFreq (1.0f)));

    for (int b = 0; b < numBins; ++b)
    {
        const float db = toDb (fftData[(size_t) b] * scale);
        auto& s = specDb[(size_t) b];
        s = db > s ? db : juce::jmax (db, s - release);
        holdDb[(size_t) b] = juce::jmax (holdDb[(size_t) b], s);

        if (b >= minBin && b <= maxBin && db > bestDb) { bestDb = db; best = b; }
    }

    if (best > 0 && bestDb > -90.0f)
    {
        const float a = toDb (fftData[(size_t) best - 1] * scale);
        const float m = bestDb;
        const float c = toDb (fftData[(size_t) best + 1] * scale);
        const float denom = a - 2.0f * m + c;
        const float p = std::abs (denom) > 1.0e-6f ? juce::jlimit (-0.5f, 0.5f, 0.5f * (a - c) / denom) : 0.0f;
        peakFreq = binToFreq ((float) best + p);
        peakDb = m - 0.25f * (a - c) * p;
    }
    else
    {
        peakFreq = 0.0f;
        peakDb = -120.0f;
    }
}

float Analyzer::spectrumAt (float freq, bool hold) const noexcept
{
    const auto& s = hold ? holdDb : specDb;
    const float bin = freq * (float) fftSize / (float) sampleRate;
    const int b0 = juce::jlimit (0, numBins - 1, (int) bin);
    const int b1 = juce::jmin (numBins - 1, b0 + 1);
    const float f = bin - (float) b0;
    return s[(size_t) b0] + f * (s[(size_t) b1] - s[(size_t) b0]);
}

juce::String Analyzer::noteName (float freq, int* centsOut)
{
    if (freq <= 0.0f)
        return "-";
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const float midi = 69.0f + 12.0f * std::log2 (freq / 440.0f);
    const int n = (int) std::round (midi);
    if (centsOut != nullptr)
        *centsOut = (int) std::round ((midi - (float) n) * 100.0f);
    return juce::String (names[((n % 12) + 12) % 12]) + juce::String (n / 12 - 1);
}
} // namespace scope
