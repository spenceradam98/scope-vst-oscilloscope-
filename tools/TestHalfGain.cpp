// Minimal FX used to verify Scope's plugin hosting: halves the signal.
#include <juce_audio_utils/juce_audio_utils.h>

class TestHalfGain : public juce::AudioProcessor
{
public:
    TestHalfGain() : AudioProcessor (BusesProperties().withInput ("In", juce::AudioChannelSet::stereo())
                                                      .withOutput ("Out", juce::AudioChannelSet::stereo())) {}
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override { b.applyGain (0.5f); }
    juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "TestHalfGain"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TestHalfGain(); }
