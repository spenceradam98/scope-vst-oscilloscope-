#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace scope
{
// Single-producer (audio thread) / single-consumer (UI thread) stereo sample FIFO.
// The audio thread never blocks or allocates: if the UI is not draining
// (editor closed), new samples are simply dropped.
class AnalysisFifo
{
public:
    static constexpr int capacity = 1 << 16;

    void push (const juce::AudioBuffer<float>& buffer) noexcept
    {
        const int n = buffer.getNumSamples();
        if (n <= 0 || buffer.getNumChannels() == 0)
            return;

        const float* l = buffer.getReadPointer (0);
        const float* r = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : l;

        const auto scope = fifo.write (juce::jmin (n, fifo.getFreeSpace()));
        if (scope.blockSize1 > 0)
        {
            std::copy (l, l + scope.blockSize1, left.begin() + scope.startIndex1);
            std::copy (r, r + scope.blockSize1, right.begin() + scope.startIndex1);
        }
        if (scope.blockSize2 > 0)
        {
            std::copy (l + scope.blockSize1, l + scope.blockSize1 + scope.blockSize2, left.begin() + scope.startIndex2);
            std::copy (r + scope.blockSize1, r + scope.blockSize1 + scope.blockSize2, right.begin() + scope.startIndex2);
        }
    }

    // Returns the number of samples copied.
    int pull (float* l, float* r, int maxSamples) noexcept
    {
        const auto scope = fifo.read (juce::jmin (maxSamples, fifo.getNumReady()));
        if (scope.blockSize1 > 0)
        {
            std::copy_n (left.begin() + scope.startIndex1, scope.blockSize1, l);
            std::copy_n (right.begin() + scope.startIndex1, scope.blockSize1, r);
        }
        if (scope.blockSize2 > 0)
        {
            std::copy_n (left.begin() + scope.startIndex2, scope.blockSize2, l + scope.blockSize1);
            std::copy_n (right.begin() + scope.startIndex2, scope.blockSize2, r + scope.blockSize1);
        }
        return scope.blockSize1 + scope.blockSize2;
    }

    void reset() noexcept { fifo.reset(); }

private:
    juce::AbstractFifo fifo { capacity };
    std::vector<float> left = std::vector<float> ((size_t) capacity), right = std::vector<float> ((size_t) capacity);
};
} // namespace scope
