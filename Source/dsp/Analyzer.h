#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

namespace scope
{
// UI-thread analysis: sample history for the scope/vector views, FFT spectrum,
// peak/RMS meters, BS.1770 loudness and stereo correlation.
class Analyzer
{
public:
    static constexpr int historySize = 1 << 15;
    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int numBins = fftSize / 2;

    Analyzer();

    void setSampleRate (double sr);
    double getSampleRate() const noexcept { return sampleRate; }

    void push (const float* l, const float* r, int n);
    void updateSpectrum();          // call once per UI frame
    void decayMeters (float seconds); // call once per UI frame
    void reset();

    // History: samplesAgo = 0 is the newest sample.
    float sample (int channel, int samplesAgo) const noexcept
    {
        const auto& h = channel == 0 ? histL : histR;
        return h[(size_t) ((writePos - 1 - samplesAgo) & (historySize - 1))];
    }
    float mid (int samplesAgo) const noexcept { return 0.5f * (sample (0, samplesAgo) + sample (1, samplesAgo)); }

    // Spectrum (dB, full-scale sine = 0 dB)
    const std::vector<float>& getSpectrumDb() const noexcept { return specDb; }
    const std::vector<float>& getPeakHoldDb() const noexcept { return holdDb; }
    float binToFreq (float bin) const noexcept { return bin * (float) sampleRate / (float) fftSize; }
    float spectrumAt (float freq, bool hold = false) const noexcept;
    float releaseDbPerSecond = 40.0f;

    // Dominant frequency
    float peakFreq = 0.0f, peakDb = -120.0f;

    // Meters (linear values unless noted)
    float peak[2] {}, peakHold[2] {}, rms[2] {};
    float peakHoldTimer[2] {};
    float vuLevel[2] {};   // RMS with VU-style 300 ms needle ballistics
    float lufsMomentary = -120.0f, lufsShortTerm = -120.0f, lufsMax = -120.0f; // LUFS
    float correlation = 0.0f;
    bool clipped[2] {};
    void resetPeaks();

    static juce::String noteName (float freq, int* centsOut = nullptr);

private:
    double sampleRate = 48000.0;
    std::vector<float> histL, histR;
    int writePos = 0;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false };
    std::vector<float> fftData, specDb, holdDb;

    // RMS / correlation (exponential averages)
    float msL = 0, msR = 0, avgLR = 0, avgLL = 0, avgRR = 0, rmsCoef = 0.001f, corrCoef = 0.001f;

    // K-weighting biquads (2 stages x 2 channels)
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        float process (float x) noexcept
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };
    Biquad shelf[2], highpass[2];
    double blockSum = 0.0; int blockCount = 0, blockLength = 4800;
    std::array<double, 30> blocks {}; int blockIndex = 0, blocksFilled = 0;
    void designKWeighting();
    void finishLoudnessBlock();
};
} // namespace scope
