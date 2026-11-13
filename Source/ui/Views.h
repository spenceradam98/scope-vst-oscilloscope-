#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"
#include "dsp/Analyzer.h"

namespace scope
{
// Base for every display: draws the themed "screen" (backdrop, bezel,
// scanlines, vignette, gloss, LCD pixelation) around the view's content.
class DisplayView : public juce::Component
{
public:
    explicit DisplayView (Analyzer& a) : analyzer (a) { setOpaque (false); }
    void setTheme (const Theme& t) { theme = &t; repaint(); }
    void paint (juce::Graphics&) override;

    // Shared helpers
    static void strokeGlow (juce::Graphics&, const juce::Path&, juce::Colour, float width, const Theme&);
    static juce::String formatDb (float db, int decimals = 1);
    static juce::String formatHz (float hz);

protected:
    virtual void paintContent (juce::Graphics&, juce::Rectangle<float> area) = 0;
    void drawReadout (juce::Graphics&, juce::Rectangle<float> area, const juce::String& text,
                      juce::Justification, float size = 13.0f, juce::Colour c = {}) const;

    Analyzer& analyzer;
    const Theme* theme = &getTheme (0);

private:
    void drawBackdrop (juce::Graphics&, juce::Rectangle<float>) const;
    void drawOverlay (juce::Graphics&, juce::Rectangle<float>) const;
};

//==============================================================================
class ScopeView : public DisplayView
{
public:
    using DisplayView::DisplayView;
    float timebaseMs = 20.0f;
    bool triggerOn = true;
    std::function<void()> onSettingsChanged;

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseDown (const juce::MouseEvent&) override;

protected:
    void paintContent (juce::Graphics&, juce::Rectangle<float>) override;
};

//==============================================================================
class SpectrumView : public DisplayView
{
public:
    using DisplayView::DisplayView;
    float floorDb = -96.0f, ceilDb = 6.0f, tiltDbPerOct = 0.0f;
    bool showHold = true;
    std::function<void()> onSettingsChanged;

    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;

protected:
    void paintContent (juce::Graphics&, juce::Rectangle<float>) override;

private:
    juce::Point<float> hover { -1.0f, -1.0f };
    juce::Rectangle<float> plot;
    float xToFreq (float x) const;
    float freqToX (float f) const;
    float dbToY (float db) const;
    float tilt (float f) const { return tiltDbPerOct * std::log2 (f / 1000.0f); }
    void drawLine (juce::Graphics&);
    void drawBands (juce::Graphics&);
    std::array<float, 31> bandHold = [] { std::array<float, 31> a {}; a.fill (-200.0f); return a; }();
};

//==============================================================================
class VectorView : public DisplayView
{
public:
    using DisplayView::DisplayView;

protected:
    void paintContent (juce::Graphics&, juce::Rectangle<float>) override;
};

//==============================================================================
class MeterView : public DisplayView
{
public:
    using DisplayView::DisplayView;
    bool compact = false;
    void mouseDown (const juce::MouseEvent&) override { analyzer.resetPeaks(); }

protected:
    void paintContent (juce::Graphics&, juce::Rectangle<float>) override;

private:
    void drawBar (juce::Graphics&, juce::Rectangle<float>, int channel);
    void drawScale (juce::Graphics&, juce::Rectangle<float>, bool leftAligned);
    void drawNeedle (juce::Graphics&, juce::Rectangle<float>, int channel);
    void drawNumbers (juce::Graphics&, juce::Rectangle<float>);
    void drawCorrelation (juce::Graphics&, juce::Rectangle<float>);
};
} // namespace scope
