#include "PluginEditor.h"

using namespace juce;

namespace
{
const char* modeNames[] = { "COMBO", "SCOPE", "SPECTRUM", "VECTOR", "METERS" };
const char* shortModeNames[] = { "ALL", "WAVE", "FFT", "XY", "LVL" };
struct SizePreset { const char* name; int w, h; };
const SizePreset sizes[] = { { "Small", 640, 400 }, { "Medium", 900, 560 }, { "Large", 1200, 740 }, { "XL", 1600, 980 } };
constexpr int headerH = 40, footerH = 40;
}

ScopeAudioProcessorEditor::ScopeAudioProcessorEditor (ScopeAudioProcessor& p)
    : AudioProcessorEditor (p), proc (p),
      chainPanel (p.chain, p.apvts),
      tapAttachment (p.apvts, "tapPre", tapButton),
      pullL ((size_t) scope::AnalysisFifo::capacity), pullR ((size_t) scope::AnalysisFifo::capacity)
{
    setLookAndFeel (&lnf);
    analyzer.setSampleRate (p.currentSampleRate.load());

    for (int i = 0; i < 5; ++i)
    {
        auto* b = modeButtons.add (new TextButton (modeNames[i]));
        b->setRadioGroupId (1001);
        b->setClickingTogglesState (true);
        b->onClick = [this, i] { setMode ((Mode) i); };
        addAndMakeVisible (b);
    }

    freezeButton.setClickingTogglesState (true);
    freezeButton.setTooltip ("Freeze the display");
    freezeButton.onClick = [this] { frozen = freezeButton.getToggleState(); };
    addAndMakeVisible (freezeButton);

    tapButton.setClickingTogglesState (true);
    tapButton.setTooltip ("On: analyse the input before the FX chain. Off: analyse the output.");
    addAndMakeVisible (tapButton);

    // Theme list grouped by era
    String lastEra;
    const auto& themes = scope::getThemes();
    for (int i = 0; i < (int) themes.size(); ++i)
    {
        if (themes[(size_t) i].era != lastEra)
        {
            themeBox.addSectionHeading (themes[(size_t) i].era);
            lastEra = themes[(size_t) i].era;
        }
        themeBox.addItem (themes[(size_t) i].name, i + 1);
    }
    themeBox.setTooltip ("Theme");
    themeBox.onChange = [this] { if (themeBox.getSelectedId() > 0) setThemeIndex (themeBox.getSelectedId() - 1); };
    addAndMakeVisible (themeBox);

    for (int i = 0; i < 4; ++i)
        sizeBox.addItem (sizes[i].name, i + 1);
    sizeBox.setTooltip ("Window size (you can also drag the corner)");
    sizeBox.onChange = [this]
    {
        const int id = sizeBox.getSelectedId();
        if (id > 0 && ! settingSize)
            setSize (sizes[id - 1].w, sizes[id - 1].h);
    };
    addAndMakeVisible (sizeBox);

    addChildComponent (scopeView);
    addChildComponent (spectrumView);
    addChildComponent (vectorView);
    addChildComponent (meterView);
    addAndMakeVisible (chainPanel);

    auto settingsChanged = [this] { saveViewSettings(); };
    scopeView.onSettingsChanged = settingsChanged;
    spectrumView.onSettingsChanged = settingsChanged;

    loadViewSettings();

    setResizable (true, true);
    setResizeLimits (560, 360, 2560, 1600);
    setSize ((int) proc.uiState.getProperty ("width", sizes[1].w), (int) proc.uiState.getProperty ("height", sizes[1].h));

    constructed = true;
    resized();
    startTimerHz (60);
}

ScopeAudioProcessorEditor::~ScopeAudioProcessorEditor()
{
    stopTimer();
    chainPanel.closeAllWindows();
    setLookAndFeel (nullptr);
}

void ScopeAudioProcessorEditor::loadViewSettings()
{
    auto& ui = proc.uiState;
    themeIndex = jlimit (0, (int) scope::getThemes().size() - 1, (int) ui.getProperty ("theme", 0));
    scopeView.timebaseMs = (float) ui.getProperty ("timebase", 20.0f);
    scopeView.triggerOn = (bool) ui.getProperty ("trigger", true);
    spectrumView.floorDb = (float) ui.getProperty ("floor", -96.0f);
    spectrumView.tiltDbPerOct = (float) ui.getProperty ("tilt", 0.0f);
    spectrumView.showHold = (bool) ui.getProperty ("hold", true);
    analyzer.releaseDbPerSecond = (float) ui.getProperty ("release", 40.0f);
    themeBox.setSelectedId (themeIndex + 1, dontSendNotification);
    applyTheme();
    setMode ((Mode) jlimit (0, 4, (int) ui.getProperty ("mode", 0)));
}

void ScopeAudioProcessorEditor::saveViewSettings()
{
    auto& ui = proc.uiState;
    ui.setProperty ("mode", (int) mode, nullptr);
    ui.setProperty ("theme", themeIndex, nullptr);
    ui.setProperty ("timebase", scopeView.timebaseMs, nullptr);
    ui.setProperty ("trigger", scopeView.triggerOn, nullptr);
    ui.setProperty ("floor", spectrumView.floorDb, nullptr);
    ui.setProperty ("tilt", spectrumView.tiltDbPerOct, nullptr);
    ui.setProperty ("hold", spectrumView.showHold, nullptr);
    ui.setProperty ("release", analyzer.releaseDbPerSecond, nullptr);
}

void ScopeAudioProcessorEditor::setMode (Mode m)
{
    mode = m;
    modeButtons[(int) m]->setToggleState (true, dontSendNotification);
    saveViewSettings();
    resized();
}

void ScopeAudioProcessorEditor::setThemeIndex (int i)
{
    themeIndex = jlimit (0, (int) scope::getThemes().size() - 1, i);
    themeBox.setSelectedId (themeIndex + 1, dontSendNotification);
    applyTheme();
    saveViewSettings();
}

void ScopeAudioProcessorEditor::applyTheme()
{
    const auto& t = scope::getTheme (themeIndex);
    lnf.setTheme (t);
    for (auto* v : std::initializer_list<scope::DisplayView*> { &scopeView, &spectrumView, &vectorView, &meterView })
        v->setTheme (t);
    chainPanel.setTheme (t);
    sendLookAndFeelChange();
    resized();
    repaint();
}

void ScopeAudioProcessorEditor::paint (Graphics& g)
{
    const auto& t = scope::getTheme (themeIndex);
    auto r = getLocalBounds();
    g.fillAll (t.chassis);

    if (t.backdrop == scope::Backdrop::BrushedMetal)
    {
        Random rng (42);
        for (int y = 0; y < getHeight(); ++y)
        {
            g.setColour ((rng.nextBool() ? Colours::white : Colours::black).withAlpha (rng.nextFloat() * 0.06f));
            g.drawHorizontalLine (y, 0.0f, (float) getWidth());
        }
    }

    if (t.bevel)
    {
        // Rack-panel bevel and screws
        g.setColour (Colours::white.withAlpha (0.25f));
        g.drawRect (r, 1);
        g.setColour (Colours::black.withAlpha (0.35f));
        g.drawHorizontalLine (headerH - 1, 0.0f, (float) getWidth());
        for (auto pt : { Point<int> (9, 9), Point<int> (getWidth() - 9, 9),
                         Point<int> (9, getHeight() - 9), Point<int> (getWidth() - 9, getHeight() - 9) })
        {
            auto s = Rectangle<float> (8.0f, 8.0f).withCentre (pt.toFloat());
            g.setColour (t.chassis.darker (0.5f));
            g.fillEllipse (s);
            g.setColour (t.chassis.brighter (0.4f));
            g.drawLine (s.getX() + 1.5f, s.getCentreY(), s.getRight() - 1.5f, s.getCentreY(), 1.2f);
        }
    }

    if (t.gloss)
    {
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.10f), 0, 0, Colours::transparentWhite, 0, (float) headerH, false));
        g.fillRect (r.removeFromTop (headerH));
    }

    // Brand mark
    auto header = getLocalBounds().removeFromTop (headerH).reduced (t.bevel ? 18 : 12, 0);
    g.setColour (t.accent);
    g.setFont (t.font (17.0f, true));
    g.drawText (String (SCOPE_PRODUCT_NAME).toUpperCase(), header, Justification::centredLeft);
    g.setColour (t.chassisText.withAlpha (0.6f));
    g.setFont (t.font (10.0f));
    g.drawText (t.era.toUpperCase() + " / " + t.name.toUpperCase(), header.withTrimmedLeft (78).withTrimmedTop (14),
                Justification::centredLeft);
}

void ScopeAudioProcessorEditor::resized()
{
    const auto& t = scope::getTheme (themeIndex);
    auto r = getLocalBounds();
    const int pad = t.bevel ? 16 : 8;

    auto header = r.removeFromTop (headerH).reduced (pad, 6);
    header.removeFromLeft (getWidth() >= 820 ? 200 : 86);
    sizeBox.setBounds (header.removeFromRight (86)); header.removeFromRight (6);
    themeBox.setBounds (header.removeFromRight (getWidth() >= 820 ? 150 : 110)); header.removeFromRight (6);
    tapButton.setBounds (header.removeFromRight (46)); header.removeFromRight (4);
    freezeButton.setBounds (header.removeFromRight (66)); header.removeFromRight (10);
    const int bw = jmin (90, (header.getWidth() - 4 * 3) / 5);
    for (int i = 0; i < modeButtons.size(); ++i)
    {
        auto* b = modeButtons[i];
        b->setButtonText (bw >= 76 ? modeNames[i] : shortModeNames[i]);
        b->setBounds (header.removeFromLeft (bw));
        header.removeFromLeft (3);
    }

    chainPanel.setBounds (r.removeFromBottom (footerH).reduced (pad - 8 + 4, 0));
    layoutDisplays (r.reduced (pad, 4));

    // Reflect free-resizing in the size box
    settingSize = true;
    int match = 0;
    for (int i = 0; i < 4; ++i)
        if (sizes[i].w == getWidth() && sizes[i].h == getHeight()) match = i + 1;
    if (match > 0) sizeBox.setSelectedId (match, dontSendNotification);
    else sizeBox.setText (String (getWidth()) + "x" + String (getHeight()), dontSendNotification);
    settingSize = false;

    if (constructed)
    {
        proc.uiState.setProperty ("width", getWidth(), nullptr);
        proc.uiState.setProperty ("height", getHeight(), nullptr);
    }
}

void ScopeAudioProcessorEditor::layoutDisplays (Rectangle<int> r)
{
    for (auto* v : std::initializer_list<Component*> { &scopeView, &spectrumView, &vectorView, &meterView })
        v->setVisible (false);
    meterView.compact = false;

    auto show = [] (Component& c, Rectangle<int> b) { c.setBounds (b); c.setVisible (true); };

    switch (mode)
    {
        case Mode::Combo:
        {
            meterView.compact = true;
            show (meterView, r.removeFromRight (jlimit (110, 150, r.getWidth() / 7)));
            r.removeFromRight (6);
            auto top = r.removeFromTop ((int) (r.getHeight() * 0.58f));
            show (spectrumView, top);
            r.removeFromTop (6);
            show (vectorView, r.removeFromRight (jmin (r.getHeight() + 10, r.getWidth() / 3)));
            r.removeFromRight (6);
            show (scopeView, r);
            break;
        }
        case Mode::Scope:    show (scopeView, r); break;
        case Mode::Spectrum: show (spectrumView, r); break;
        case Mode::Vector:   show (vectorView, r); break;
        case Mode::Meters:   show (meterView, r); break;
    }
}

void ScopeAudioProcessorEditor::timerCallback()
{
    const double sr = proc.currentSampleRate.load();
    if (std::abs (sr - analyzer.getSampleRate()) > 0.5)
        analyzer.setSampleRate (sr);

    const int n = proc.fifo.pull (pullL.data(), pullR.data(), (int) pullL.size());
    const auto now = Time::getMillisecondCounter();
    const float dt = lastFrameMs == 0 ? 1.0f / 60.0f : jlimit (0.001f, 0.25f, (float) (now - lastFrameMs) * 0.001f);
    lastFrameMs = now;

    if (frozen)
        return;

    if (n > 0)
        analyzer.push (pullL.data(), pullR.data(), n);
    analyzer.decayMeters (dt);
    if (spectrumView.isVisible() || scopeView.isVisible())
        analyzer.updateSpectrum();

    for (auto* v : std::initializer_list<Component*> { &scopeView, &spectrumView, &vectorView, &meterView })
        if (v->isVisible())
            v->repaint();
}
