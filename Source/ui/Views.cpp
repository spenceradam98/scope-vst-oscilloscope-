#include "Views.h"

namespace scope
{
using namespace juce;

static constexpr float minFreq = 20.0f, maxFreq = 20000.0f;
static constexpr float meterMinDb = -60.0f, meterMaxDb = 6.0f;

static float gainToDb (float g) { return g > 1.0e-6f ? 20.0f * std::log10 (g) : -120.0f; }

//==============================================================================
// DisplayView
//==============================================================================
void DisplayView::paint (Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const auto& t = *theme;
    const float r = t.cornerRadius;

    // Bezel
    g.setColour (t.screenEdge);
    g.fillRoundedRectangle (bounds, r + 2.0f);
    auto screen = bounds.reduced (t.bevel ? 4.0f : 2.0f);

    Path clip;
    clip.addRoundedRectangle (screen, r);

    {
        Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (clip);
        g.setColour (t.screen);
        g.fillRect (screen);
        drawBackdrop (g, screen);

        if (t.pixelated && screen.getWidth() > 4 && screen.getHeight() > 4)
        {
            // Render at half resolution and scale up for chunky LCD pixels.
            const int w = (int) screen.getWidth() / 2, h = (int) screen.getHeight() / 2;
            Image img (Image::ARGB, jmax (1, w), jmax (1, h), true);
            {
                Graphics ig (img);
                ig.addTransform (AffineTransform::scale (0.5f));
                paintContent (ig, screen.withPosition (0, 0));
            }
            g.setImageResamplingQuality (Graphics::lowResamplingQuality);
            g.drawImage (img, screen);
        }
        else
        {
            Graphics::ScopedSaveState s2 (g);
            g.setOrigin (screen.getTopLeft().toInt());
            paintContent (g, screen.withPosition (0, 0));
        }

        drawOverlay (g, screen);
    }

    if (t.bevel)
    {
        // Sunken screen: dark top-left, light bottom-right
        g.setColour (Colours::black.withAlpha (0.5f));
        g.drawLine (screen.getX(), screen.getY(), screen.getRight(), screen.getY(), 2.0f);
        g.drawLine (screen.getX(), screen.getY(), screen.getX(), screen.getBottom(), 2.0f);
        g.setColour (Colours::white.withAlpha (0.35f));
        g.drawLine (screen.getX(), screen.getBottom(), screen.getRight(), screen.getBottom(), 1.5f);
        g.drawLine (screen.getRight(), screen.getY(), screen.getRight(), screen.getBottom(), 1.5f);
    }
}

void DisplayView::drawBackdrop (Graphics& g, Rectangle<float> a) const
{
    const auto& t = *theme;
    switch (t.backdrop)
    {
        case Backdrop::PerspectiveGrid:
        {
            // Sky glow above the horizon, then a receding floor grid
            const float horizon = a.getY() + a.getHeight() * 0.62f;
            g.setGradientFill (ColourGradient (t.screen, a.getX(), a.getY(),
                                               t.accent.withAlpha (0.22f), a.getX(), horizon, false));
            g.fillRect (a.withBottom (horizon));
            g.setColour (t.accent.withAlpha (0.35f));
            g.drawHorizontalLine ((int) horizon, a.getX(), a.getRight());

            g.setColour (t.grid.withMultipliedAlpha (0.8f));
            const float cx = a.getCentreX();
            for (int i = -14; i <= 14; ++i)
            {
                const float xb = cx + (float) i * a.getWidth() * 0.12f;
                g.drawLine (cx + (float) i * a.getWidth() * 0.012f, horizon, xb, a.getBottom(), 1.0f);
            }
            for (int i = 1; i < 12; ++i)
            {
                const float k = (float) i / 12.0f;
                const float y = horizon + (a.getBottom() - horizon) * k * k;
                g.drawHorizontalLine ((int) y, a.getX(), a.getRight());
            }
            break;
        }
        case Backdrop::HexGrid:
        {
            g.setColour (t.grid.withMultipliedAlpha (0.6f));
            const float s = 16.0f, hgt = s * std::sqrt (3.0f);
            for (float y = a.getY() - hgt; y < a.getBottom() + hgt; y += hgt)
                for (int col = 0; col * s * 1.5f < a.getWidth() + 2 * s; ++col)
                {
                    const float x = a.getX() + (float) col * s * 1.5f;
                    const float yy = y + ((col & 1) ? hgt * 0.5f : 0.0f);
                    Path hexagon;
                    hexagon.addPolygon ({ x, yy }, 6, s * 0.95f, MathConstants<float>::halfPi / 3.0f);
                    g.strokePath (hexagon, PathStrokeType (0.6f));
                }
            break;
        }
        case Backdrop::BrushedMetal:
        case Backdrop::None:
            break;
    }
}

void DisplayView::drawOverlay (Graphics& g, Rectangle<float> a) const
{
    const auto& t = *theme;
    if (t.scanlines)
    {
        g.setColour (Colours::black.withAlpha (0.22f));
        for (float y = a.getY(); y < a.getBottom(); y += 3.0f)
            g.fillRect (a.getX(), y, a.getWidth(), 1.0f);
    }
    if (t.vignette)
    {
        ColourGradient vg (Colours::transparentBlack, a.getCentreX(), a.getCentreY(),
                           Colours::black.withAlpha (0.55f), a.getX(), a.getY(), true);
        vg.addColour (0.6, Colours::transparentBlack);
        g.setGradientFill (vg);
        g.fillRect (a);
    }
    if (t.gloss)
    {
        Path p;
        p.startNewSubPath (a.getTopLeft());
        p.lineTo (a.getTopRight());
        p.lineTo (a.getRight(), a.getY() + a.getHeight() * 0.32f);
        p.quadraticTo (a.getCentreX(), a.getY() + a.getHeight() * 0.48f, a.getX(), a.getY() + a.getHeight() * 0.36f);
        p.closeSubPath();
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.16f), a.getX(), a.getY(),
                                           Colours::white.withAlpha (0.02f), a.getX(), a.getY() + a.getHeight() * 0.45f, false));
        g.fillPath (p);
    }
}

void DisplayView::strokeGlow (Graphics& g, const Path& p, Colour c, float width, const Theme& t)
{
    if (t.glow > 0.0f)
    {
        g.setColour (c.withMultipliedAlpha (0.07f * t.glow));
        g.strokePath (p, PathStrokeType (width * 7.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (c.withMultipliedAlpha (0.18f * t.glow));
        g.strokePath (p, PathStrokeType (width * 3.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    g.setColour (c);
    g.strokePath (p, PathStrokeType (width, PathStrokeType::curved, PathStrokeType::rounded));
}

String DisplayView::formatDb (float db, int decimals)
{
    if (db <= -119.0f) return "-inf";
    return (db > 0.0f ? "+" : "") + String (db, decimals);
}

String DisplayView::formatHz (float hz)
{
    if (hz <= 0.0f) return "-";
    return hz >= 1000.0f ? String (hz / 1000.0f, hz >= 10000.0f ? 1 : 2) + " kHz" : String (hz, 1) + " Hz";
}

void DisplayView::drawReadout (Graphics& g, Rectangle<float> area, const String& text,
                               Justification j, float size, Colour c) const
{
    g.setFont (theme->font (size));
    g.setColour (c.isTransparent() ? theme->text : c);
    g.drawText (text, area, j, false);
}

//==============================================================================
// ScopeView
//==============================================================================
void ScopeView::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    timebaseMs = jlimit (1.0f, 200.0f, timebaseMs * (w.deltaY > 0 ? 0.85f : 1.0f / 0.85f));
    if (onSettingsChanged) onSettingsChanged();
}

void ScopeView::mouseDown (const MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
        return;
    PopupMenu m;
    m.addSectionHeader ("Timebase (or use the mouse wheel)");
    const float bases[] = { 2, 5, 10, 20, 50, 100, 200 };
    for (int i = 0; i < 7; ++i)
        m.addItem (String ((int) bases[i]) + " ms", true, std::abs (timebaseMs - bases[i]) < 0.5f,
                   [this, b = bases[i]] { timebaseMs = b; if (onSettingsChanged) onSettingsChanged(); });
    m.addSeparator();
    m.addItem ("Trigger on rising edge", true, triggerOn, [this] { triggerOn = ! triggerOn; if (onSettingsChanged) onSettingsChanged(); });
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withMousePosition());
}

void ScopeView::paintContent (Graphics& g, Rectangle<float> a)
{
    const auto& t = *theme;
    auto plot = a.reduced (10.0f, 8.0f).withTrimmedBottom (18.0f);

    // Graticule: 10 x 8 divisions
    g.setColour (t.grid);
    for (int i = 0; i <= 10; ++i)
        g.drawVerticalLine ((int) (plot.getX() + plot.getWidth() * (float) i / 10.0f), plot.getY(), plot.getBottom());
    for (int i = 0; i <= 8; ++i)
        g.drawHorizontalLine ((int) (plot.getY() + plot.getHeight() * (float) i / 8.0f), plot.getX(), plot.getRight());
    if (t.minorTicks)
    {
        g.setColour (t.grid.withMultipliedAlpha (1.6f));
        for (int i = 0; i <= 50; ++i)
        {
            const float x = plot.getX() + plot.getWidth() * (float) i / 50.0f;
            g.drawVerticalLine ((int) x, plot.getCentreY() - 3.0f, plot.getCentreY() + 3.0f);
        }
        for (int i = 0; i <= 40; ++i)
        {
            const float y = plot.getY() + plot.getHeight() * (float) i / 40.0f;
            g.drawHorizontalLine ((int) y, plot.getCentreX() - 3.0f, plot.getCentreX() + 3.0f);
        }
    }

    // Window and trigger
    const double sr = analyzer.getSampleRate();
    const int searchMax = Analyzer::historySize / 2;
    const int window = jlimit (16, searchMax, (int) (timebaseMs * 0.001 * sr));
    int start = window; // samplesAgo of the left edge

    if (triggerOn)
    {
        const int searchLen = jmin (searchMax, jmax (window, (int) (sr * 0.05)));
        const float hyst = 0.002f;
        for (int ago = window; ago < window + searchLen; ++ago)
        {
            const float now = analyzer.mid (ago), before = analyzer.mid (ago + 1);
            if (before < 0.0f && now >= 0.0f && (now - before) > hyst)
            {
                start = ago;
                break;
            }
        }
    }

    const float cy = plot.getCentreY(), halfH = plot.getHeight() * 0.5f;
    auto buildPath = [&] (int ch)
    {
        Path p;
        const int cols = jmax (1, (int) plot.getWidth());
        if (window > cols * 2)
        {
            // Min/max per pixel column so nothing is hidden when zoomed out
            for (int x = 0; x < cols; ++x)
            {
                const int s0 = start - (int) ((int64) x * window / cols);
                const int s1 = start - (int) ((int64) (x + 1) * window / cols);
                float lo = 1.0e9f, hi = -1.0e9f;
                for (int s = s0; s > s1; --s)
                {
                    const float v = analyzer.sample (ch, jmax (0, s));
                    lo = jmin (lo, v); hi = jmax (hi, v);
                }
                const float px = plot.getX() + (float) x;
                const float y0 = cy - jlimit (-1.2f, 1.2f, hi) * halfH, y1 = cy - jlimit (-1.2f, 1.2f, lo) * halfH;
                if (x == 0) p.startNewSubPath (px, y0); else p.lineTo (px, y0);
                p.lineTo (px, y1);
            }
        }
        else
        {
            for (int i = 0; i <= window; ++i)
            {
                const float px = plot.getX() + plot.getWidth() * (float) i / (float) window;
                const float v = analyzer.sample (ch, jmax (0, start - i));
                const float py = cy - jlimit (-1.2f, 1.2f, v) * halfH;
                if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
            }
        }
        return p;
    };

    {
        Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (plot.expanded (2.0f).toNearestInt());
        strokeGlow (g, buildPath (1), t.traceR.withMultipliedAlpha (0.85f), t.traceWidth, t);
        strokeGlow (g, buildPath (0), t.traceL, t.traceWidth, t);
    }

    // Labels and readouts
    g.setFont (t.font (11.0f));
    g.setColour (t.gridText);
    g.drawText ("+1", plot.getX() + 3, plot.getY() + 1, 30, 12, Justification::left);
    g.drawText ("0", plot.getX() + 3, cy - 13, 30, 12, Justification::left);
    g.drawText ("-1", plot.getX() + 3, plot.getBottom() - 13, 30, 12, Justification::left);

    auto bottom = a.removeFromBottom (20.0f).reduced (10.0f, 2.0f);
    const float div = timebaseMs / 10.0f;
    const String leftText = (div < 1.0f ? String (div * 1000.0f, 0) + " us" : String (div, div < 10.0f ? 1 : 0) + " ms") + "/div"
                            + (triggerOn ? "  TRIG" : "  FREE");

    int cents = 0;
    auto note = Analyzer::noteName (analyzer.peakFreq, &cents);
    String levels, pitch;
    levels << "L " << formatDb (gainToDb (analyzer.peakHold[0])) << "  R " << formatDb (gainToDb (analyzer.peakHold[1])) << " dBFS";
    if (analyzer.peakFreq > 0.0f)
        pitch << formatHz (analyzer.peakFreq) << " " << note << (cents >= 0 ? " +" : " ") << cents << "c";

    // Show as much as fits: timebase always, then pitch, then levels.
    g.setFont (t.font (12.0f));
    auto widthOf = [&] (const String& str) { return GlyphArrangement::getStringWidth (g.getCurrentFont(), str) + 16.0f; };
    const float avail = bottom.getWidth() - widthOf (leftText);
    String right;
    if (widthOf (levels + "   " + pitch) <= avail) right = levels + "   " + pitch;
    else if (widthOf (pitch) <= avail) right = pitch;
    drawReadout (g, bottom, leftText, Justification::left, 12.0f, t.gridText);
    drawReadout (g, bottom, right, Justification::right, 12.0f);

    drawReadout (g, a.reduced (12.0f, 8.0f).removeFromTop (14.0f), "L", Justification::right, 12.0f, t.traceL);
    drawReadout (g, a.reduced (24.0f, 8.0f).removeFromTop (14.0f), "R", Justification::right, 12.0f, t.traceR);
}

//==============================================================================
// SpectrumView
//==============================================================================
float SpectrumView::xToFreq (float x) const
{
    return minFreq * std::pow (maxFreq / minFreq, (x - plot.getX()) / plot.getWidth());
}

float SpectrumView::freqToX (float f) const
{
    return plot.getX() + plot.getWidth() * std::log (f / minFreq) / std::log (maxFreq / minFreq);
}

float SpectrumView::dbToY (float db) const
{
    return jmap (db, floorDb, ceilDb, plot.getBottom(), plot.getY());
}

void SpectrumView::mouseMove (const MouseEvent& e) { hover = e.position; }
void SpectrumView::mouseExit (const MouseEvent&) { hover = { -1.0f, -1.0f }; }

void SpectrumView::mouseDown (const MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
        return;
    auto changed = [this] { if (onSettingsChanged) onSettingsChanged(); };
    PopupMenu tiltMenu, rangeMenu, speedMenu;
    for (float s : { 0.0f, 3.0f, 4.5f, 6.0f })
        tiltMenu.addItem (String (s, 1) + " dB/oct", true, approximatelyEqual (tiltDbPerOct, s), [this, s, changed] { tiltDbPerOct = s; changed(); });
    for (float f : { -60.0f, -78.0f, -96.0f, -120.0f })
        rangeMenu.addItem (String ((int) f) + " dB floor", true, approximatelyEqual (floorDb, f), [this, f, changed] { floorDb = f; changed(); });
    const std::pair<const char*, float> speeds[] = { { "Fast", 120.0f }, { "Medium", 40.0f }, { "Slow", 12.0f } };
    for (auto& sp : speeds)
        speedMenu.addItem (sp.first, true, approximatelyEqual (analyzer.releaseDbPerSecond, sp.second),
                           [this, v = sp.second, changed] { analyzer.releaseDbPerSecond = v; changed(); });

    PopupMenu m;
    m.addSubMenu ("Tilt", tiltMenu);
    m.addSubMenu ("Range", rangeMenu);
    m.addSubMenu ("Release", speedMenu);
    m.addItem ("Show peak hold", true, showHold, [this, changed] { showHold = ! showHold; changed(); });
    m.addItem ("Reset peak hold", [this] { analyzer.resetPeaks(); });
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withMousePosition());
}

void SpectrumView::paintContent (Graphics& g, Rectangle<float> a)
{
    const auto& t = *theme;
    plot = a.reduced (8.0f).withTrimmedLeft (28.0f).withTrimmedBottom (14.0f).withTrimmedTop (18.0f);

    // Grid
    g.setFont (t.font (10.5f));
    for (float f : { 20.f, 30.f, 40.f, 50.f, 60.f, 70.f, 80.f, 90.f, 100.f, 200.f, 300.f, 400.f, 500.f, 600.f, 700.f, 800.f, 900.f,
                     1000.f, 2000.f, 3000.f, 4000.f, 5000.f, 6000.f, 7000.f, 8000.f, 9000.f, 10000.f, 20000.f })
    {
        const auto lead = f / std::pow (10.0f, std::floor (std::log10 (f)));
        const bool major = approximatelyEqual (lead, 1.0f) || approximatelyEqual (lead, 2.0f) || approximatelyEqual (lead, 5.0f);
        const float x = freqToX (f);
        g.setColour (major ? t.grid : t.grid.withMultipliedAlpha (0.45f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        if (major)
        {
            g.setColour (t.gridText);
            g.drawText (f >= 1000.f ? String ((int) (f / 1000)) + "k" : String ((int) f),
                        Rectangle<float> (x - 20, plot.getBottom() + 1, 40, 13), Justification::centred);
        }
    }
    for (float db = 0.0f; db >= floorDb; db -= (floorDb < -80.0f ? 12.0f : 6.0f))
    {
        const float y = dbToY (db);
        g.setColour (t.grid);
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (t.gridText);
        g.drawText (String ((int) db), Rectangle<float> (a.getX() + 2, y - 7, 32, 14), Justification::centredRight);
    }

    {
        Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (plot.toNearestInt());
        if (t.spectrumStyle == SpectrumStyle::Bands) drawBands (g); else drawLine (g);
    }

    // Dominant peak marker
    if (analyzer.peakFreq > 0.0f)
    {
        const float px = freqToX (analyzer.peakFreq);
        const float py = dbToY (analyzer.peakDb + tilt (analyzer.peakFreq));
        Path tri;
        tri.addTriangle (px - 5, py - 12, px + 5, py - 12, px, py - 4);
        g.setColour (t.marker);
        g.fillPath (tri);
    }

    // Header readout
    int cents = 0;
    auto note = Analyzer::noteName (analyzer.peakFreq, &cents);
    auto header = a.reduced (10.0f, 3.0f).removeFromTop (16.0f).withTrimmedLeft (26.0f);
    String peakText = "PEAK  " + formatHz (analyzer.peakFreq);
    if (analyzer.peakFreq > 0.0f)
        peakText << "  " << note << (cents >= 0 ? " +" : " ") << cents << "c  " << formatDb (analyzer.peakDb) << " dB";
    drawReadout (g, header, peakText, Justification::left, 12.5f, t.marker);
    drawReadout (g, header, (tiltDbPerOct > 0 ? "TILT " + String (tiltDbPerOct, 1) + " dB/oct   " : String()) + "FFT 4096",
                 Justification::right, 11.0f, t.gridText);

    // Cursor readout
    if (plot.contains (hover))
    {
        const float f = xToFreq (hover.x);
        const float db = analyzer.spectrumAt (f) + tilt (f);
        int c = 0;
        auto n = Analyzer::noteName (f, &c);
        g.setColour (t.text.withAlpha (0.35f));
        g.drawVerticalLine ((int) hover.x, plot.getY(), plot.getBottom());
        g.drawHorizontalLine ((int) dbToY (db), plot.getX(), plot.getRight());

        String s;
        s << formatHz (f) << "  " << n << (c >= 0 ? " +" : " ") << c << "c  " << formatDb (db) << " dB";
        g.setFont (t.font (12.0f));
        const float w = GlyphArrangement::getStringWidth (g.getCurrentFont(), s) + 14.0f;
        auto box = Rectangle<float> (hover.x + 10, hover.y - 26, w, 20);
        if (box.getRight() > plot.getRight()) box.setX (hover.x - w - 10);
        if (box.getY() < plot.getY()) box.setY (hover.y + 8);
        g.setColour (t.screen.withAlpha (0.85f));
        g.fillRoundedRectangle (box, 3.0f);
        g.setColour (t.text.withAlpha (0.6f));
        g.drawRoundedRectangle (box, 3.0f, 1.0f);
        g.setColour (t.text);
        g.drawText (s, box, Justification::centred);
    }
}

void SpectrumView::drawLine (Graphics& g)
{
    const auto& t = *theme;
    Path line, fill, hold;
    const int cols = (int) plot.getWidth();
    const auto& spec = analyzer.getSpectrumDb();
    const auto& holdDb = analyzer.getPeakHoldDb();
    const float binHz = analyzer.binToFreq (1.0f);

    for (int i = 0; i <= cols; ++i)
    {
        const float x = plot.getX() + (float) i;
        const float f0 = xToFreq (x), f1 = xToFreq (x + 1.0f);
        float v, h;
        const int b0 = (int) (f0 / binHz), b1 = (int) (f1 / binHz);
        if (b1 > b0 + 1)
        {
            v = h = -200.0f;
            for (int b = b0; b <= jmin (b1, Analyzer::numBins - 1); ++b)
            {
                v = jmax (v, spec[(size_t) b]);
                h = jmax (h, holdDb[(size_t) b]);
            }
        }
        else
        {
            v = analyzer.spectrumAt (f0);
            h = analyzer.spectrumAt (f0, true);
        }
        const float y = dbToY (v + tilt (f0));
        const float yh = dbToY (h + tilt (f0));
        if (i == 0) { line.startNewSubPath (x, y); hold.startNewSubPath (x, yh); }
        else        { line.lineTo (x, y); hold.lineTo (x, yh); }
    }

    fill = line;
    fill.lineTo (plot.getRight(), plot.getBottom() + 2);
    fill.lineTo (plot.getX(), plot.getBottom() + 2);
    fill.closeSubPath();
    g.setGradientFill (ColourGradient (t.spectrumFill, 0, plot.getY(),
                                       t.spectrumFill.withMultipliedAlpha (0.2f), 0, plot.getBottom(), false));
    g.fillPath (fill);

    if (showHold)
    {
        g.setColour (t.peakHold);
        g.strokePath (hold, PathStrokeType (1.0f));
    }
    strokeGlow (g, line, t.spectrum, t.traceWidth, t);
}

void SpectrumView::drawBands (Graphics& g)
{
    // 31-band third-octave LED ladder (ISO centre frequencies)
    static const float centres[31] = { 20, 25, 31.5f, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630,
                                       800, 1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000, 20000 };
    const auto& t = *theme;
    const auto& spec = analyzer.getSpectrumDb();
    const float binHz = analyzer.binToFreq (1.0f);
    const float colW = plot.getWidth() / 31.0f;
    const float segH = jlimit (3.0f, 8.0f, plot.getHeight() / 40.0f);
    const int numSegs = (int) (plot.getHeight() / segH);
    const float k = std::pow (2.0f, 1.0f / 6.0f);

    for (int b = 0; b < 31; ++b)
    {
        const float lo = centres[b] / k, hi = centres[b] * k;
        float v = -200.0f;
        const int b0 = jmax (0, (int) (lo / binHz)), b1 = jmin (Analyzer::numBins - 1, (int) (hi / binHz) + 1);
        for (int i = b0; i <= b1; ++i)
            v = jmax (v, spec[(size_t) i]);
        if (b1 - b0 < 1)
            v = analyzer.spectrumAt (centres[b]);
        v += tilt (centres[b]);
        bandHold[(size_t) b] = jmax (bandHold[(size_t) b] - 0.4f, v);

        const float x = plot.getX() + (float) b * colW + colW * 0.12f;
        const float w = colW * 0.76f;
        const float level = jlimit (0.0f, 1.0f, jmap (v, floorDb, ceilDb, 0.0f, 1.0f));
        const float holdLevel = jlimit (0.0f, 1.0f, jmap (bandHold[(size_t) b], floorDb, ceilDb, 0.0f, 1.0f));
        const int lit = (int) (level * (float) numSegs);
        const int holdSeg = (int) (holdLevel * (float) numSegs);

        for (int s = 0; s < numSegs; ++s)
        {
            const float frac = (float) s / (float) numSegs;
            const float db = floorDb + frac * (ceilDb - floorDb);
            auto on = db > -6.0f ? t.meterHigh : db > -18.0f ? t.meterMid : t.meterLow;
            const bool isOn = s < lit || (showHold && s == holdSeg && holdSeg > 0);
            const float y = plot.getBottom() - (float) (s + 1) * segH;
            g.setColour (isOn ? on : t.meterOff);
            g.fillRect (x, y + 1.0f, w, segH - 1.5f);
            if (isOn && t.glow > 0.0f)
            {
                g.setColour (on.withAlpha (0.12f * t.glow));
                g.fillRect (x - 2.0f, y - 1.0f, w + 4.0f, segH + 2.0f);
            }
        }
    }
}

//==============================================================================
// VectorView (stereo goniometer)
//==============================================================================
void VectorView::paintContent (Graphics& g, Rectangle<float> a)
{
    const auto& t = *theme;
    auto corr = a.removeFromBottom (22.0f).reduced (12.0f, 5.0f);
    auto sq = a.reduced (10.0f);
    const float size = jmin (sq.getWidth(), sq.getHeight());
    sq = sq.withSizeKeepingCentre (size, size);
    const float r = size * 0.5f;
    const auto c = sq.getCentre();

    g.setColour (t.grid);
    g.drawEllipse (sq, 1.0f);
    g.drawEllipse (sq.reduced (r * 0.5f), 1.0f);
    g.drawLine (c.x, sq.getY(), c.x, sq.getBottom());
    g.drawLine (sq.getX(), c.y, sq.getRight(), c.y);
    const float d = r * 0.7071f;
    g.drawLine (c.x - d, c.y - d, c.x + d, c.y + d);
    g.drawLine (c.x + d, c.y - d, c.x - d, c.y + d);

    g.setFont (t.font (11.0f));
    g.setColour (t.gridText);
    g.drawText ("M", Rectangle<float> (c.x - 10, sq.getY() + 2, 20, 12), Justification::centred);
    g.drawText ("L", Rectangle<float> (c.x - d - 14, c.y - d - 14, 14, 12), Justification::centred);
    g.drawText ("R", Rectangle<float> (c.x + d, c.y - d - 14, 14, 12), Justification::centred);
    g.drawText ("+S", Rectangle<float> (sq.getX() + 2, c.y - 14, 20, 12), Justification::left);
    g.drawText ("-S", Rectangle<float> (sq.getRight() - 22, c.y - 14, 20, 12), Justification::right);

    // Samples: x = side, y = mid (mono = vertical line)
    const int n = 2048;
    Path p;
    for (int i = n - 1; i >= 0; --i)
    {
        const float L = analyzer.sample (0, i), R = analyzer.sample (1, i);
        const float x = c.x + jlimit (-1.0f, 1.0f, (R - L) * 0.7071f) * r;
        const float y = c.y - jlimit (-1.0f, 1.0f, (L + R) * 0.7071f) * r;
        if (i == n - 1) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    if (t.glow > 0.0f)
    {
        g.setColour (t.traceL.withAlpha (0.08f * t.glow));
        g.strokePath (p, PathStrokeType (4.0f));
    }
    g.setColour (t.traceL.withAlpha (0.65f));
    g.strokePath (p, PathStrokeType (1.0f));

    // Correlation bar
    const float cv = analyzer.correlation;
    g.setColour (t.meterOff);
    g.fillRoundedRectangle (corr, 2.0f);
    const float mid = corr.getCentreX();
    const float x = mid + cv * corr.getWidth() * 0.5f;
    g.setColour (cv < 0.0f ? t.meterHigh : t.meterLow);
    g.fillRect (Rectangle<float> (jmin (mid, x), corr.getY(), std::abs (x - mid), corr.getHeight()));
    g.setColour (t.gridText);
    g.drawVerticalLine ((int) mid, corr.getY() - 2, corr.getBottom() + 2);
    g.setFont (t.font (10.5f));
    g.setColour (t.text);
    g.drawText ("-1", corr, Justification::left);
    g.drawText ("+1", corr, Justification::right);
    g.drawText (String (cv, 2), corr, Justification::centred);
}

//==============================================================================
// MeterView
//==============================================================================
void MeterView::paintContent (Graphics& g, Rectangle<float> a)
{
    const auto& t = *theme;
    a = a.reduced (8.0f);

    if (compact)
    {
        auto nums = a.removeFromBottom (74.0f);
        auto corrArea = a.removeFromBottom (18.0f);
        a.removeFromBottom (6.0f);
        auto scale = a.removeFromLeft (24.0f);
        const float w = (a.getWidth() - 6.0f) * 0.5f;
        drawBar (g, a.removeFromLeft (w), 0);
        a.removeFromLeft (6.0f);
        drawBar (g, a.removeFromLeft (w), 1);
        drawScale (g, scale.withY (a.getY()).withHeight (a.getHeight()), false);
        drawCorrelation (g, corrArea);
        drawNumbers (g, nums);
        return;
    }

    auto right = a.removeFromRight (jmin (300.0f, a.getWidth() * 0.42f));
    a.removeFromRight (12.0f);

    if (t.meterStyle == MeterStyle::Needle)
    {
        const float w = (a.getWidth() - 10.0f) * 0.5f;
        drawNeedle (g, a.removeFromLeft (w), 0);
        a.removeFromLeft (10.0f);
        drawNeedle (g, a.removeFromLeft (w), 1);
    }
    else
    {
        auto barsArea = a.withSizeKeepingCentre (jmin (a.getWidth(), 260.0f), a.getHeight());
        auto scale = barsArea.removeFromLeft (34.0f);
        const float w = (barsArea.getWidth() - 10.0f) * 0.5f;
        auto top = barsArea.removeFromTop (16.0f);
        drawReadout (g, top.removeFromLeft (w), "L", Justification::centred, 12.0f, t.traceL);
        drawReadout (g, top.removeFromRight (w), "R", Justification::centred, 12.0f, t.traceR);
        drawBar (g, barsArea.removeFromLeft (w), 0);
        barsArea.removeFromLeft (10.0f);
        drawBar (g, barsArea.removeFromLeft (w), 1);
        drawScale (g, scale.withY (barsArea.getY()).withHeight (barsArea.getHeight()), false);
    }

    auto corrArea = right.removeFromBottom (40.0f);
    drawCorrelation (g, corrArea.removeFromBottom (20.0f));
    drawReadout (g, corrArea, "CORRELATION", Justification::bottomLeft, 11.0f, t.gridText);
    drawNumbers (g, right);
}

static float meterNorm (float db) { return jlimit (0.0f, 1.0f, (db - meterMinDb) / (meterMaxDb - meterMinDb)); }

void MeterView::drawBar (Graphics& g, Rectangle<float> r, int ch)
{
    const auto& t = *theme;
    const float pk = gainToDb (analyzer.peak[ch]);
    const float rm = gainToDb (analyzer.rms[ch]);
    const float hold = gainToDb (analyzer.peakHold[ch]);
    auto colourFor = [&] (float db) { return db > -3.0f ? t.meterHigh : db > -12.0f ? t.meterMid : t.meterLow; };

    // Clip LED on top
    auto clip = r.removeFromTop (6.0f);
    g.setColour (analyzer.clipped[ch] ? t.meterHigh : t.meterOff);
    g.fillRect (clip);
    r.removeFromTop (3.0f);

    if (t.meterStyle == MeterStyle::Segments || t.meterStyle == MeterStyle::Needle)
    {
        const float segH = jlimit (3.0f, 7.0f, r.getHeight() / 44.0f);
        const int n = (int) (r.getHeight() / segH);
        for (int s = 0; s < n; ++s)
        {
            const float db = meterMinDb + ((float) s + 0.5f) / (float) n * (meterMaxDb - meterMinDb);
            const bool onPeak = db <= pk, onRms = db <= rm;
            const float y = r.getBottom() - (float) (s + 1) * segH;
            auto c = colourFor (db);
            g.setColour (onRms ? c : onPeak ? c.withMultipliedAlpha (0.55f) : t.meterOff);
            g.fillRect (r.getX(), y + 0.75f, r.getWidth(), segH - 1.5f);
            if (onPeak && t.glow > 0.0f)
            {
                g.setColour (c.withAlpha (0.1f * t.glow));
                g.fillRect (r.getX() - 2, y - 1, r.getWidth() + 4, segH + 2);
            }
        }
    }
    else
    {
        g.setColour (t.meterOff);
        g.fillRect (r);
        ColourGradient grad (t.meterLow, 0, r.getBottom(), t.meterHigh, 0, r.getY(), false);
        grad.addColour (meterNorm (-12.0f), t.meterMid);
        grad.addColour (meterNorm (-3.0f), t.meterHigh);
        g.setGradientFill (grad);
        const float pkY = r.getBottom() - meterNorm (pk) * r.getHeight();
        const float rmY = r.getBottom() - meterNorm (rm) * r.getHeight();
        g.setOpacity (0.45f);
        g.fillRect (r.withTop (pkY));
        g.setOpacity (1.0f);
        g.fillRect (r.withTop (rmY));
    }

    const float hY = r.getBottom() - meterNorm (hold) * r.getHeight();
    g.setColour (colourFor (hold).brighter (0.3f));
    g.fillRect (r.getX(), hY - 1.0f, r.getWidth(), 2.0f);
}

void MeterView::drawScale (Graphics& g, Rectangle<float> r, bool)
{
    const auto& t = *theme;
    r.removeFromTop (9.0f);
    g.setFont (t.font (10.0f));
    g.setColour (t.gridText);
    for (float db : { 6.f, 0.f, -6.f, -12.f, -18.f, -24.f, -30.f, -40.f, -50.f, -60.f })
    {
        const float y = r.getBottom() - meterNorm (db) * r.getHeight();
        g.drawText (String ((int) db), Rectangle<float> (r.getX(), y - 6, r.getWidth() - 3, 12), Justification::centredRight);
    }
}

void MeterView::drawNeedle (Graphics& g, Rectangle<float> r, int ch)
{
    // Classic VU: 0 VU = -18 dBFS RMS, scale -20..+3 VU, linear in voltage.
    const auto& t = *theme;
    auto face = r.withSizeKeepingCentre (r.getWidth(), jmin (r.getHeight(), r.getWidth() * 0.75f));
    g.setColour (t.screen.brighter (0.06f));
    g.fillRoundedRectangle (face, 6.0f);
    g.setColour (t.gridText.withAlpha (0.5f));
    g.drawRoundedRectangle (face, 6.0f, 1.0f);

    const Point<float> pivot (face.getCentreX(), face.getBottom() - face.getHeight() * 0.08f);
    const float radius = jmin (face.getHeight() * 0.62f, face.getWidth() * 0.4f);
    const float span = degreesToRadians (48.0f);
    auto vuToPos = [] (float vu)
    {
        const float lo = std::pow (10.0f, -20.0f / 20.0f), hi = std::pow (10.0f, 3.0f / 20.0f);
        return jlimit (-0.02f, 1.04f, (std::pow (10.0f, vu / 20.0f) - lo) / (hi - lo));
    };
    auto angleFor = [&] (float pos) { return -span + pos * 2.0f * span; };
    auto pointAt = [&] (float angle, float rad) { return pivot + Point<float> (std::sin (angle), -std::cos (angle)) * rad; };

    // Red zone
    Path red;
    red.addCentredArc (pivot.x, pivot.y, radius, radius, 0.0f, angleFor (vuToPos (0.0f)), angleFor (vuToPos (3.0f)), true);
    g.setColour (t.meterHigh);
    g.strokePath (red, PathStrokeType (4.0f));
    Path arc;
    arc.addCentredArc (pivot.x, pivot.y, radius, radius, 0.0f, angleFor (0.0f), angleFor (vuToPos (0.0f)), true);
    g.setColour (t.gridText);
    g.strokePath (arc, PathStrokeType (1.5f));

    g.setFont (t.font (jmax (9.0f, face.getHeight() * 0.07f)));
    for (float vu : { -20.f, -10.f, -7.f, -5.f, -3.f, -2.f, -1.f, 0.f, 1.f, 2.f, 3.f })
    {
        const float ang = angleFor (vuToPos (vu));
        g.setColour (vu > 0.0f ? t.meterHigh : t.gridText);
        g.drawLine (Line<float> (pointAt (ang, radius), pointAt (ang, radius + 7.0f)), 1.5f);
        const auto lp = pointAt (ang, radius + 16.0f);
        g.drawText (String ((int) std::abs (vu)), Rectangle<float> (lp.x - 12, lp.y - 7, 24, 14), Justification::centred);
    }
    g.setColour (t.text);
    g.setFont (t.font (jmax (12.0f, face.getHeight() * 0.11f), true));
    g.drawText ("VU", face.withTrimmedTop (face.getHeight() * 0.45f).withHeight (face.getHeight() * 0.2f), Justification::centred);
    g.setFont (t.font (11.0f));
    g.drawText (ch == 0 ? "LEFT" : "RIGHT", face.reduced (8).removeFromBottom (14), Justification::bottomLeft);

    const float ang = angleFor (vuToPos (gainToDb (analyzer.vuLevel[ch]) + 18.0f));
    g.setColour (Colours::black.withAlpha (0.2f));
    g.drawLine (Line<float> (pivot.translated (2, 2), pointAt (ang, radius + 4).translated (2, 2)), 2.0f);
    g.setColour (t.traceL);
    g.drawLine (Line<float> (pivot, pointAt (ang, radius + 4)), 1.8f);
    g.fillEllipse (Rectangle<float> (10, 10).withCentre (pivot));

    // Peak LED
    auto led = Rectangle<float> (10, 10).withCentre ({ face.getRight() - 16, face.getY() + 16 });
    g.setColour (analyzer.peak[ch] > 0.89f ? t.meterHigh : t.meterHigh.withAlpha (0.18f));
    g.fillEllipse (led);
}

void MeterView::drawNumbers (Graphics& g, Rectangle<float> r)
{
    const auto& t = *theme;
    auto row = [&] (const String& label, const String& value, Colour c, float size)
    {
        auto line = r.removeFromTop (size + 8.0f);
        g.setFont (t.font (10.5f));
        g.setColour (t.gridText);
        g.drawText (label, line.removeFromTop (12.0f), Justification::left);
        g.setFont (t.font (size, true));
        g.setColour (c);
        g.drawText (value, line, Justification::left);
    };
    auto pkL = formatDb (gainToDb (analyzer.peakHold[0])), pkR = formatDb (gainToDb (analyzer.peakHold[1]));
    auto rmL = formatDb (gainToDb (analyzer.rms[0])), rmR = formatDb (gainToDb (analyzer.rms[1]));

    if (compact)
    {
        row ("PEAK dBFS", pkL + " " + pkR, t.text, 12.0f);
        row ("LUFS M / S", formatDb (analyzer.lufsMomentary) + " " + formatDb (analyzer.lufsShortTerm), t.text, 12.0f);
        return;
    }

    const float big = jlimit (14.0f, 30.0f, r.getHeight() / 9.0f);
    row ("MOMENTARY LUFS", formatDb (analyzer.lufsMomentary), t.text, big);
    row ("SHORT-TERM LUFS", formatDb (analyzer.lufsShortTerm), t.text, big);
    row ("MAX MOMENTARY", formatDb (analyzer.lufsMax), t.text.withMultipliedAlpha (0.8f), big * 0.7f);
    row ("PEAK HOLD  L / R  dBFS", pkL + "  /  " + pkR,
         (analyzer.clipped[0] || analyzer.clipped[1]) ? t.meterHigh : t.text, big * 0.7f);
    row ("RMS  L / R  dBFS", rmL + "  /  " + rmR, t.text, big * 0.7f);
    g.setFont (t.font (10.0f));
    g.setColour (t.gridText.withMultipliedAlpha (0.8f));
    g.drawText ("click to reset peaks", r.removeFromTop (14.0f), Justification::left);
}

void MeterView::drawCorrelation (Graphics& g, Rectangle<float> r)
{
    const auto& t = *theme;
    const float cv = analyzer.correlation;
    g.setColour (t.meterOff);
    g.fillRect (r);
    const float mid = r.getCentreX(), x = mid + cv * r.getWidth() * 0.5f;
    g.setColour (cv < 0.0f ? t.meterHigh : t.meterLow);
    g.fillRect (Rectangle<float> (jmin (mid, x), r.getY(), std::abs (x - mid), r.getHeight()));
    g.setColour (t.text);
    g.setFont (t.font (10.0f));
    g.drawText (String (cv, 2), r, Justification::centred);
}
} // namespace scope
