#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// A theme is a look inspired by a family of real hardware from one era.
// Colours plus a handful of rendering traits (glow, scanlines, bevels, LED
// segments, needle meters...). Add a theme by adding an entry to getThemes().
namespace scope
{
enum class MeterStyle    { Bars, Segments, Needle };
enum class SpectrumStyle { Line, Bands };
enum class Backdrop      { None, PerspectiveGrid, HexGrid, BrushedMetal };

struct Theme
{
    juce::String name, era, blurb;

    // Chassis / UI
    juce::Colour chassis, chassisText, accent, buttonFace, buttonText;
    // Display (the "screen")
    juce::Colour screen, screenEdge, grid, gridText, text;
    // Traces
    juce::Colour traceL, traceR, spectrum, spectrumFill, peakHold, marker;
    // Meters
    juce::Colour meterLow, meterMid, meterHigh, meterOff;

    float glow = 0.0f;           // 0..1 phosphor/neon bloom around traces
    float traceWidth = 1.5f;
    float cornerRadius = 4.0f;   // screen corners
    bool scanlines = false;      // CRT raster lines
    bool vignette = false;       // CRT curvature darkening
    bool bevel = false;          // 3D raised/sunken panels
    bool gloss = false;          // glossy highlight over the screen
    bool monoFont = false;       // terminal/instrument lettering
    bool minorTicks = false;     // lab-scope graticule ticks
    bool pixelated = false;      // chunky LCD/home computer pixels
    MeterStyle meterStyle = MeterStyle::Bars;
    SpectrumStyle spectrumStyle = SpectrumStyle::Line;
    Backdrop backdrop = Backdrop::None;

    juce::Font font (float height, bool bold = false) const
    {
        auto opts = juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain);
        if (monoFont)
            opts = opts.withName (juce::Font::getDefaultMonospacedFontName());
        return juce::Font (opts);
    }
};

inline juce::Colour hex (juce::uint32 rgb, float alpha = 1.0f)
{
    return juce::Colour (0xff000000 | rgb).withAlpha (alpha);
}

inline const std::vector<Theme>& getThemes()
{
    static const std::vector<Theme> themes = []
    {
        std::vector<Theme> t;

        { // Green P31 phosphor lab oscilloscope, blue-grey enamel chassis
            Theme x; x.name = "Lab Scope"; x.era = "1970s";
            x.blurb = "Green phosphor CRT in a blue-grey test-bench chassis";
            x.chassis = hex (0x4a5a66); x.chassisText = hex (0xe8ecef); x.accent = hex (0x7fffa0);
            x.buttonFace = hex (0x5d6f7c); x.buttonText = hex (0xf2f4f5);
            x.screen = hex (0x061a0c); x.screenEdge = hex (0x1c2a22); x.grid = hex (0x2f6b45, 0.55f);
            x.gridText = hex (0x5fbf80); x.text = hex (0x8dffb0);
            x.traceL = hex (0x66ff99); x.traceR = hex (0x3fe0a0); x.spectrum = hex (0x66ff99);
            x.spectrumFill = hex (0x2bff7a, 0.12f); x.peakHold = hex (0x2f9f60); x.marker = hex (0xd8ffe4);
            x.meterLow = hex (0x55ee88); x.meterMid = hex (0xc8ff70); x.meterHigh = hex (0xff7755); x.meterOff = hex (0x0f2a18);
            x.glow = 0.85f; x.traceWidth = 1.6f; x.cornerRadius = 18.0f;
            x.scanlines = false; x.vignette = true; x.monoFont = true; x.minorTicks = true;
            x.meterStyle = MeterStyle::Bars;
            t.push_back (x);
        }
        { // Backlit VU meters, walnut and brushed aluminium
            Theme x; x.name = "Studio VU"; x.era = "1970s";
            x.blurb = "Warm backlit VU needles on a walnut console";
            x.chassis = hex (0x3b2417); x.chassisText = hex (0xf3e3c3); x.accent = hex (0xffb347);
            x.buttonFace = hex (0x5a3a25); x.buttonText = hex (0xf6e7cc);
            x.screen = hex (0xf1ddb0); x.screenEdge = hex (0x1a120c); x.grid = hex (0x6b4a2a, 0.35f);
            x.gridText = hex (0x5a3a20); x.text = hex (0x2a1a0e);
            x.traceL = hex (0x1d1410); x.traceR = hex (0xb0281e); x.spectrum = hex (0x1d1410);
            x.spectrumFill = hex (0x9b6a35, 0.18f); x.peakHold = hex (0xb0281e, 0.7f); x.marker = hex (0xb0281e);
            x.meterLow = hex (0x1d1410); x.meterMid = hex (0x1d1410); x.meterHigh = hex (0xc0301f); x.meterOff = hex (0xe2c992);
            x.traceWidth = 1.8f; x.cornerRadius = 6.0f; x.bevel = true; x.vignette = true;
            x.meterStyle = MeterStyle::Needle;
            t.push_back (x);
        }
        { // 31-band rack analyser with LED ladders
            Theme x; x.name = "LED Rack"; x.era = "1980s";
            x.blurb = "Black 19-inch rack unit with red, amber and green LED ladders";
            x.chassis = hex (0x141414); x.chassisText = hex (0xd9d9d9); x.accent = hex (0xff3b30);
            x.buttonFace = hex (0x2a2a2a); x.buttonText = hex (0xe6e6e6);
            x.screen = hex (0x050505); x.screenEdge = hex (0x2b2b2b); x.grid = hex (0x3a3a3a, 0.6f);
            x.gridText = hex (0x9a9a9a); x.text = hex (0xff5040);
            x.traceL = hex (0x39ff14); x.traceR = hex (0xffb000); x.spectrum = hex (0x39ff14);
            x.spectrumFill = hex (0x39ff14, 0.1f); x.peakHold = hex (0xff3b30); x.marker = hex (0xffd000);
            x.meterLow = hex (0x2bff2b); x.meterMid = hex (0xffc400); x.meterHigh = hex (0xff2a1a); x.meterOff = hex (0x1c1c1c);
            x.glow = 0.35f; x.cornerRadius = 2.0f; x.bevel = true; x.monoFont = true;
            x.meterStyle = MeterStyle::Segments; x.spectrumStyle = SpectrumStyle::Bands;
            t.push_back (x);
        }
        { // Vacuum fluorescent display, teal-cyan segments
            Theme x; x.name = "VFD Deck"; x.era = "1980s";
            x.blurb = "Cyan vacuum-fluorescent display behind smoked glass";
            x.chassis = hex (0x1b1d22); x.chassisText = hex (0xbfc6cf); x.accent = hex (0x4ff5e0);
            x.buttonFace = hex (0x2b2f36); x.buttonText = hex (0xd0d6de);
            x.screen = hex (0x03100f); x.screenEdge = hex (0x0b0b0d); x.grid = hex (0x1b5a55, 0.5f);
            x.gridText = hex (0x2fb5a6); x.text = hex (0x6ffff0);
            x.traceL = hex (0x5ffff0); x.traceR = hex (0xff8f3f); x.spectrum = hex (0x5ffff0);
            x.spectrumFill = hex (0x5ffff0, 0.08f); x.peakHold = hex (0xff8f3f); x.marker = hex (0xff8f3f);
            x.meterLow = hex (0x5ffff0); x.meterMid = hex (0x5ffff0); x.meterHigh = hex (0xff6f3f); x.meterOff = hex (0x0c2422);
            x.glow = 0.7f; x.cornerRadius = 3.0f; x.monoFont = true;
            x.meterStyle = MeterStyle::Segments; x.spectrumStyle = SpectrumStyle::Bands;
            t.push_back (x);
        }
        { // Grey rack gear with reflective monochrome LCD
            Theme x; x.name = "Grey LCD"; x.era = "1990s";
            x.blurb = "Bevelled grey rack gear with a reflective monochrome LCD";
            x.chassis = hex (0xb7b7b2); x.chassisText = hex (0x1e1e1e); x.accent = hex (0x2b4f8a);
            x.buttonFace = hex (0xcfcfca); x.buttonText = hex (0x111111);
            x.screen = hex (0x9fae8a); x.screenEdge = hex (0x6d6d68); x.grid = hex (0x6f7d5c, 0.55f);
            x.gridText = hex (0x2f3a24); x.text = hex (0x1d2614);
            x.traceL = hex (0x1d2614); x.traceR = hex (0x3e4e2c); x.spectrum = hex (0x1d2614);
            x.spectrumFill = hex (0x1d2614, 0.18f); x.peakHold = hex (0x4c5c38); x.marker = hex (0x111a0a);
            x.meterLow = hex (0x1d2614); x.meterMid = hex (0x1d2614); x.meterHigh = hex (0x1d2614); x.meterOff = hex (0x8e9c7a);
            x.cornerRadius = 0.0f; x.bevel = true; x.pixelated = true; x.monoFont = true; x.traceWidth = 2.0f;
            x.meterStyle = MeterStyle::Segments;
            t.push_back (x);
        }
        { // Beige desktop software era: teal desktop, grey 3D widgets
            Theme x; x.name = "Beige Desktop"; x.era = "1990s";
            x.blurb = "Early desktop audio software: grey 3D widgets, navy title bars";
            x.chassis = hex (0xc0c0c0); x.chassisText = hex (0x000000); x.accent = hex (0x000080);
            x.buttonFace = hex (0xc0c0c0); x.buttonText = hex (0x000000);
            x.screen = hex (0x000000); x.screenEdge = hex (0x808080); x.grid = hex (0x008080, 0.6f);
            x.gridText = hex (0x00c0c0); x.text = hex (0xffff00);
            x.traceL = hex (0x00ff00); x.traceR = hex (0xff00ff); x.spectrum = hex (0x00ff00);
            x.spectrumFill = hex (0x00ff00, 0.15f); x.peakHold = hex (0xffff00); x.marker = hex (0xffffff);
            x.meterLow = hex (0x00c000); x.meterMid = hex (0xffff00); x.meterHigh = hex (0xff0000); x.meterOff = hex (0x202020);
            x.cornerRadius = 0.0f; x.bevel = true; x.pixelated = true;
            t.push_back (x);
        }
        { // Glossy brushed aluminium and blue LCD
            Theme x; x.name = "Brushed Aqua"; x.era = "2000s";
            x.blurb = "Brushed aluminium with a glossy blue-backlit LCD";
            x.chassis = hex (0xa9b0b8); x.chassisText = hex (0x1b2530); x.accent = hex (0x2f8cff);
            x.buttonFace = hex (0xd6dce2); x.buttonText = hex (0x16202a);
            x.screen = hex (0x0b2a52); x.screenEdge = hex (0x4c5560); x.grid = hex (0x6fb6ff, 0.22f);
            x.gridText = hex (0x9fd0ff); x.text = hex (0xe6f4ff);
            x.traceL = hex (0x9fe6ff); x.traceR = hex (0xffd27a); x.spectrum = hex (0xbfefff);
            x.spectrumFill = hex (0x5fbfff, 0.25f); x.peakHold = hex (0xffd27a); x.marker = hex (0xffffff);
            x.meterLow = hex (0x5fd0ff); x.meterMid = hex (0xbff07a); x.meterHigh = hex (0xff6a4a); x.meterOff = hex (0x123a66);
            x.glow = 0.25f; x.cornerRadius = 10.0f; x.gloss = true; x.backdrop = Backdrop::BrushedMetal;
            t.push_back (x);
        }
        { // Late-2000s glossy black plugin GUI with orange accents
            Theme x; x.name = "Carbon Gloss"; x.era = "2000s";
            x.blurb = "Piano-black gloss and carbon weave with orange highlights";
            x.chassis = hex (0x1a1a1c); x.chassisText = hex (0xd8d8d8); x.accent = hex (0xff7a1a);
            x.buttonFace = hex (0x2c2c30); x.buttonText = hex (0xeeeeee);
            x.screen = hex (0x0c0c0e); x.screenEdge = hex (0x3a3a3e); x.grid = hex (0x444448, 0.5f);
            x.gridText = hex (0x8a8a90); x.text = hex (0xffa04a);
            x.traceL = hex (0xff8a2a); x.traceR = hex (0x4ab8ff); x.spectrum = hex (0xff8a2a);
            x.spectrumFill = hex (0xff7a1a, 0.18f); x.peakHold = hex (0xffffff, 0.5f); x.marker = hex (0xffffff);
            x.meterLow = hex (0xff8a2a); x.meterMid = hex (0xffc04a); x.meterHigh = hex (0xff3a2a); x.meterOff = hex (0x202022);
            x.glow = 0.3f; x.cornerRadius = 8.0f; x.gloss = true;
            t.push_back (x);
        }
        { // Contemporary flat dark plugin
            Theme x; x.name = "Studio Dark"; x.era = "Modern";
            x.blurb = "Flat, quiet dark UI for long sessions";
            x.chassis = hex (0x16181d); x.chassisText = hex (0xc9ced6); x.accent = hex (0x7aa2ff);
            x.buttonFace = hex (0x22262e); x.buttonText = hex (0xdfe3ea);
            x.screen = hex (0x0f1115); x.screenEdge = hex (0x262a33); x.grid = hex (0x2a2f3a, 0.9f);
            x.gridText = hex (0x6b7385); x.text = hex (0xe6e9ef);
            x.traceL = hex (0x7aa2ff); x.traceR = hex (0xff8fa3); x.spectrum = hex (0x9ab8ff);
            x.spectrumFill = hex (0x7aa2ff, 0.16f); x.peakHold = hex (0xffffff, 0.35f); x.marker = hex (0xffd166);
            x.meterLow = hex (0x5ad19a); x.meterMid = hex (0xffd166); x.meterHigh = hex (0xff6b6b); x.meterOff = hex (0x1d2027);
            x.cornerRadius = 6.0f;
            t.push_back (x);
        }
        { // Contemporary flat light
            Theme x; x.name = "Studio Light"; x.era = "Modern";
            x.blurb = "Clean flat light UI";
            x.chassis = hex (0xeef0f3); x.chassisText = hex (0x23262c); x.accent = hex (0x2563eb);
            x.buttonFace = hex (0xffffff); x.buttonText = hex (0x23262c);
            x.screen = hex (0xffffff); x.screenEdge = hex (0xd5d9e0); x.grid = hex (0xe3e6eb);
            x.gridText = hex (0x8a919e); x.text = hex (0x1f2328);
            x.traceL = hex (0x2563eb); x.traceR = hex (0xe11d48); x.spectrum = hex (0x2563eb);
            x.spectrumFill = hex (0x2563eb, 0.12f); x.peakHold = hex (0x94a3b8); x.marker = hex (0xea580c);
            x.meterLow = hex (0x16a34a); x.meterMid = hex (0xeab308); x.meterHigh = hex (0xdc2626); x.meterOff = hex (0xe9ecf0);
            x.cornerRadius = 6.0f;
            t.push_back (x);
        }
        { // Retro-futuristic neon on a perspective grid
            Theme x; x.name = "Synthwave"; x.era = "Retro-future";
            x.blurb = "Magenta and cyan neon over an endless perspective grid";
            x.chassis = hex (0x140a24); x.chassisText = hex (0xf3d9ff); x.accent = hex (0xff3cac);
            x.buttonFace = hex (0x24123e); x.buttonText = hex (0xf8e6ff);
            x.screen = hex (0x0d0420); x.screenEdge = hex (0x5a1d7a); x.grid = hex (0xff3cac, 0.18f);
            x.gridText = hex (0xc77dff); x.text = hex (0x5ff4ff);
            x.traceL = hex (0x2de2e6); x.traceR = hex (0xff3cac); x.spectrum = hex (0x2de2e6);
            x.spectrumFill = hex (0xff3cac, 0.14f); x.peakHold = hex (0xffd319); x.marker = hex (0xffd319);
            x.meterLow = hex (0x2de2e6); x.meterMid = hex (0xff3cac); x.meterHigh = hex (0xffd319); x.meterOff = hex (0x1e0d36);
            x.glow = 1.0f; x.traceWidth = 2.0f; x.cornerRadius = 8.0f;
            x.backdrop = Backdrop::PerspectiveGrid; x.meterStyle = MeterStyle::Segments;
            t.push_back (x);
        }
        { // Starship computer terminal: amber monochrome raster
            Theme x; x.name = "Amber Terminal"; x.era = "Retro-future";
            x.blurb = "Amber raster terminal from a cargo-ship computer core";
            x.chassis = hex (0x1c1813); x.chassisText = hex (0xd9b779); x.accent = hex (0xffb000);
            x.buttonFace = hex (0x2a241b); x.buttonText = hex (0xffc94a);
            x.screen = hex (0x120b02); x.screenEdge = hex (0x050403); x.grid = hex (0x7a4f00, 0.5f);
            x.gridText = hex (0xb37a00); x.text = hex (0xffc23a);
            x.traceL = hex (0xffb000); x.traceR = hex (0xff7a00); x.spectrum = hex (0xffb000);
            x.spectrumFill = hex (0xffb000, 0.12f); x.peakHold = hex (0x996600); x.marker = hex (0xfff0c0);
            x.meterLow = hex (0xffb000); x.meterMid = hex (0xffb000); x.meterHigh = hex (0xff5a00); x.meterOff = hex (0x2a1a04);
            x.glow = 0.8f; x.cornerRadius = 14.0f; x.scanlines = true; x.vignette = true; x.monoFont = true;
            x.meterStyle = MeterStyle::Segments;
            t.push_back (x);
        }
        { // Holographic HUD
            Theme x; x.name = "Hologram"; x.era = "Retro-future";
            x.blurb = "Translucent cyan heads-up display on a hex lattice";
            x.chassis = hex (0x050b12); x.chassisText = hex (0x9fe8ff); x.accent = hex (0x38d6ff);
            x.buttonFace = hex (0x0b1a26); x.buttonText = hex (0xbff1ff);
            x.screen = hex (0x02080d); x.screenEdge = hex (0x1b6f8f); x.grid = hex (0x38d6ff, 0.14f);
            x.gridText = hex (0x5fbfdf); x.text = hex (0xbff1ff);
            x.traceL = hex (0x7ff0ff); x.traceR = hex (0xb39dff); x.spectrum = hex (0x7ff0ff);
            x.spectrumFill = hex (0x38d6ff, 0.12f); x.peakHold = hex (0xb39dff); x.marker = hex (0xffffff);
            x.meterLow = hex (0x38d6ff); x.meterMid = hex (0x7ff0ff); x.meterHigh = hex (0xff5f8f); x.meterOff = hex (0x0b1d28);
            x.glow = 0.9f; x.cornerRadius = 2.0f; x.scanlines = true; x.monoFont = true;
            x.backdrop = Backdrop::HexGrid; x.meterStyle = MeterStyle::Segments;
            t.push_back (x);
        }
        return t;
    }();
    return themes;
}

inline const Theme& getTheme (int index)
{
    const auto& t = getThemes();
    return t[(size_t) juce::jlimit (0, (int) t.size() - 1, index)];
}

//==============================================================================
// Styles JUCE widgets from a Theme.
class ThemedLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void setTheme (const Theme& t)
    {
        theme = &t;
        setColour (juce::ResizableWindow::backgroundColourId, t.chassis);
        setColour (juce::TextButton::buttonColourId, t.buttonFace);
        setColour (juce::TextButton::buttonOnColourId, t.accent);
        setColour (juce::TextButton::textColourOffId, t.buttonText);
        setColour (juce::TextButton::textColourOnId, t.accent.contrasting (0.9f));
        setColour (juce::ComboBox::backgroundColourId, t.buttonFace);
        setColour (juce::ComboBox::textColourId, t.buttonText);
        setColour (juce::ComboBox::outlineColourId, t.screenEdge);
        setColour (juce::ComboBox::arrowColourId, t.buttonText);
        setColour (juce::PopupMenu::backgroundColourId, t.buttonFace);
        setColour (juce::PopupMenu::textColourId, t.buttonText);
        setColour (juce::PopupMenu::headerTextColourId, t.accent);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, t.accent);
        setColour (juce::PopupMenu::highlightedTextColourId, t.accent.contrasting (0.9f));
        setColour (juce::Label::textColourId, t.chassisText);
        setColour (juce::Slider::thumbColourId, t.accent);
        setColour (juce::Slider::trackColourId, t.buttonFace);
        setColour (juce::Slider::rotarySliderFillColourId, t.accent);
        setColour (juce::Slider::rotarySliderOutlineColourId, t.buttonFace);
        setColour (juce::Slider::textBoxTextColourId, t.chassisText);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ToggleButton::textColourId, t.chassisText);
        setColour (juce::ToggleButton::tickColourId, t.accent);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int h) override
    {
        return theme ? theme->font (juce::jmin (14.0f, (float) h * 0.55f), true) : juce::Font (juce::FontOptions (13.0f));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return theme ? theme->font (13.0f) : juce::Font (juce::FontOptions (13.0f));
    }

    juce::Font getPopupMenuFont() override
    {
        return theme ? theme->font (14.0f) : juce::Font (juce::FontOptions (14.0f));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& bg,
                               bool over, bool down) override
    {
        if (theme == nullptr || ! theme->bevel)
        {
            juce::LookAndFeel_V4::drawButtonBackground (g, b, bg, over, down);
            return;
        }

        auto r = b.getLocalBounds().toFloat();
        auto face = b.getToggleState() ? theme->accent : bg;
        if (over) face = face.brighter (0.08f);
        g.setColour (face);
        g.fillRect (r);

        const bool sunken = down || b.getToggleState();
        auto light = juce::Colours::white.withAlpha (0.7f);
        auto dark = juce::Colours::black.withAlpha (0.55f);
        g.setColour (sunken ? dark : light);
        g.fillRect (r.removeFromTop (1.5f));
        g.fillRect (b.getLocalBounds().toFloat().removeFromLeft (1.5f));
        g.setColour (sunken ? light : dark);
        g.fillRect (b.getLocalBounds().toFloat().removeFromBottom (1.5f));
        g.fillRect (b.getLocalBounds().toFloat().removeFromRight (1.5f));
    }

    const Theme* theme = nullptr;
};
} // namespace scope
