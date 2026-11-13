#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/AnalysisFifo.h"
#include "hosting/PluginChain.h"

class ScopeAudioProcessor : public juce::AudioProcessor,
                            private juce::Timer
{
public:
    ScopeAudioProcessor();
    ~ScopeAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::ValueTree uiState { "UI" };   // mode, theme, size, view settings
    scope::AnalysisFifo fifo;
    scope::PluginChain chain;
    std::atomic<double> currentSampleRate { 48000.0 };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void timerCallback() override;

    std::atomic<float>* outputGainDb = nullptr;
    std::atomic<float>* chainBypass = nullptr;
    std::atomic<float>* tapPre = nullptr;
    juce::LinearSmoothedValue<float> gain;
    std::unique_ptr<juce::XmlElement> pendingChainState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopeAudioProcessor)
};
