#pragma once
#include "PluginProcessor.h"
#include "ui/Theme.h"
#include "ui/Views.h"
#include "ui/ChainPanel.h"

class ScopeAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    enum class Mode { Combo, Scope, Spectrum, Vector, Meters };

    explicit ScopeAudioProcessorEditor (ScopeAudioProcessor&);
    ~ScopeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Used by the screenshot tool and tests
    void setMode (Mode);
    void setThemeIndex (int);
    void pumpAnalysis() { timerCallback(); }

private:
    void timerCallback() override;
    void applyTheme();
    void saveViewSettings();
    void loadViewSettings();
    void layoutDisplays (juce::Rectangle<int>);

    ScopeAudioProcessor& proc;
    scope::Analyzer analyzer;
    scope::ThemedLookAndFeel lnf;
    int themeIndex = 0;
    Mode mode = Mode::Combo;
    bool frozen = false;

    scope::ScopeView scopeView { analyzer };
    scope::SpectrumView spectrumView { analyzer };
    scope::VectorView vectorView { analyzer };
    scope::MeterView meterView { analyzer };
    scope::ChainPanel chainPanel;

    juce::OwnedArray<juce::TextButton> modeButtons;
    juce::TextButton freezeButton { "FREEZE" }, tapButton { "PRE" };
    juce::ComboBox themeBox, sizeBox;
    juce::AudioProcessorValueTreeState::ButtonAttachment tapAttachment;
    juce::TooltipWindow tooltips { this, 600 };
    std::vector<float> pullL, pullR;
    juce::uint32 lastFrameMs = 0;
    bool settingSize = false, constructed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopeAudioProcessorEditor)
};
