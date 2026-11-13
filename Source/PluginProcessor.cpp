#include "PluginProcessor.h"
#include "PluginEditor.h"

ScopeAudioProcessor::ScopeAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    outputGainDb = apvts.getRawParameterValue ("outputGain");
    chainBypass = apvts.getRawParameterValue ("chainBypass");
    tapPre = apvts.getRawParameterValue ("tapPre");
    startTimerHz (4);
}

ScopeAudioProcessor::~ScopeAudioProcessor()
{
    stopTimer();
}

juce::AudioProcessorValueTreeState::ParameterLayout ScopeAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "outputGain", 1 }, "Output Gain",
                                                       NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
                                                       AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "chainBypass", 1 }, "FX Chain Bypass", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "tapPre", 1 }, "Analyse Pre-Chain", false));
    return layout;
}

void ScopeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    chain.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    gain.reset (sampleRate, 0.02);
    gain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputGainDb->load()));
    fifo.reset();
}

void ScopeAudioProcessor::releaseResources()
{
    chain.release();
}

bool ScopeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void ScopeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const bool pre = tapPre->load() > 0.5f;
    if (pre)
        fifo.push (buffer);

    if (chainBypass->load() < 0.5f && chain.size() > 0)
        chain.process (buffer, midi);

    gain.setTargetValue (juce::Decibels::decibelsToGain (outputGainDb->load()));
    if (gain.isSmoothing() || std::abs (gain.getTargetValue() - 1.0f) > 1.0e-6f)
        gain.applyGain (buffer, buffer.getNumSamples());

    if (! pre)
        fifo.push (buffer);
}

void ScopeAudioProcessor::timerCallback()
{
    if (pendingChainState != nullptr)
    {
        chain.fromXml (*pendingChainState);
        pendingChainState.reset();
    }

    const int latency = chainBypass->load() > 0.5f ? 0 : chain.getLatencySamples();
    if (latency != getLatencySamples())
        setLatencySamples (latency);
}

juce::AudioProcessorEditor* ScopeAudioProcessor::createEditor()
{
    return new ScopeAudioProcessorEditor (*this);
}

void ScopeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement root ("SCOPE");
    root.setAttribute ("version", 1);
    if (auto params = apvts.copyState().createXml())
        root.addChildElement (params.release());
    if (auto ui = uiState.createXml())
        root.addChildElement (ui.release());
    root.addChildElement (chain.toXml().release());
    copyXmlToBinary (root, destData);
}

void ScopeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto root = getXmlFromBinary (data, sizeInBytes);
    if (root == nullptr || ! root->hasTagName ("SCOPE"))
        return;

    if (auto* params = root->getChildByName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*params));
    if (auto* ui = root->getChildByName ("UI"))
        uiState.copyPropertiesFrom (juce::ValueTree::fromXml (*ui), nullptr);

    if (auto* c = root->getChildByName ("CHAIN"))
    {
        // Plugins must be created on the message thread.
        if (juce::MessageManager::getInstance()->isThisTheMessageThread())
            chain.fromXml (*c);
        else
            pendingChainState = std::make_unique<juce::XmlElement> (*c);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ScopeAudioProcessor();
}
