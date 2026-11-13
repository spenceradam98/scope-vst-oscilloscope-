#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace scope
{
// A serial chain of hosted plugins built on juce::AudioProcessorGraph.
// Audio thread: process(). Everything else: message thread only.
class PluginChain : public juce::ChangeBroadcaster
{
public:
    PluginChain();
    ~PluginChain() override;

    void prepare (double sampleRate, int blockSize, int numChannels);
    void release();
    void process (juce::AudioBuffer<float>&, juce::MidiBuffer&);

    // Editing (message thread)
    bool addPlugin (const juce::PluginDescription&, juce::String& error);
    void removePlugin (int index);
    void movePlugin (int from, int to);
    void setBypassed (int index, bool bypassed);
    bool isBypassed (int index) const;
    void clear();

    int size() const noexcept { return (int) slots.size(); }
    juce::AudioPluginInstance* getPlugin (int index) const;
    int getLatencySamples() const { return graph.getLatencySamples(); }

    std::unique_ptr<juce::XmlElement> toXml() const;
    void fromXml (const juce::XmlElement&);

    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;
    juce::File getPluginListFile() const;
    juce::File getDeadMansPedalFile() const;
    void saveKnownPlugins() const;

private:
    struct Slot
    {
        juce::AudioProcessorGraph::Node::Ptr node;
    };

    juce::AudioProcessorGraph graph;
    juce::AudioProcessorGraph::Node::Ptr inputNode, outputNode;
    std::vector<Slot> slots;
    double currentRate = 44100.0;
    int currentBlock = 512, currentChannels = 2;

    void rewire();
    void connect (juce::AudioProcessorGraph::NodeID src, int srcChans, juce::AudioProcessorGraph::NodeID dst, int dstChans);
    void configureBuses (juce::AudioPluginInstance&) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginChain)
};
} // namespace scope
