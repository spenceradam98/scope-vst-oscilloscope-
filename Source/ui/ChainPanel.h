#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "hosting/PluginChain.h"
#include "Theme.h"

namespace scope
{
// Footer strip that shows the hosted FX chain and lets the user edit it.
class ChainPanel : public juce::Component, private juce::ChangeListener
{
public:
    ChainPanel (PluginChain&, juce::AudioProcessorValueTreeState&);
    ~ChainPanel() override;

    void setTheme (const Theme& t) { theme = &t; repaint(); }
    void resized() override;
    void paint (juce::Graphics&) override;
    void closeAllWindows();
    void mouseDown (const juce::MouseEvent&) override;

private:
    class PluginWindow;
    class ScannerWindow;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void rebuild();
    void showAddMenu();
    void showSlotMenu (int index);
    void openEditor (int index);
    void closeEditor (juce::AudioProcessor*);
    void openScanner();

    PluginChain& chain;
    const Theme* theme = &getTheme (0);
    juce::Label title { {}, "FX CHAIN" };
    juce::TextButton bypassButton { "BYPASS" }, addButton { "+ ADD FX" }, scanButton { "PLUGINS..." };
    juce::Slider outGain { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Label outLabel { {}, "OUT" };
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment gainAttachment;
    juce::OwnedArray<juce::TextButton> slotButtons;
    std::vector<std::unique_ptr<PluginWindow>> windows;
    std::unique_ptr<ScannerWindow> scanner;
    bool suppressClick = false;
};
} // namespace scope
