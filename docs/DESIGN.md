# Scope: design plan

Working name **Scope** by **YourBrand** (placeholders, set in `CMakeLists.txt`).

An analyser plugin that sits on a track as an effect. Audio goes through an
optional chain of hosted plugins, and the result (or the dry input) is shown
on several display types with numeric readouts.

## Goals for v0.1

- Display types: **Combo**, **Scope** (waveform), **Spectrum**, **Vector**
  (stereo goniometer) and **Meters** (peak, RMS, LUFS, correlation).
- Readouts: dominant frequency with note name and cents, cursor frequency/dB
  on the spectrum, peak and RMS dBFS, momentary and short-term LUFS, max LUFS,
  stereo correlation.
- Themes modelled on real hardware eras (see below). Size presets S/M/L/XL
  plus free resizing; mode, theme and size are saved with the session.
- FX hosting: load VST3 (and AU on macOS) plugins into a serial chain inside
  Scope. Per-slot bypass, reorder, remove, open the plugin's own editor.
  Chain bypass, and a tap switch to analyse **pre** or **post** chain.

## Architecture

```
DAW in ─► [Pre tap] ─► PluginChain (AudioProcessorGraph) ─► Output gain ─► [Post tap] ─► DAW out
                                                                 │
                                       AnalysisFifo (lock-free, audio → UI)
                                                                 │
                         Analyzer (message thread: history, FFT, meters, LUFS)
                                                                 │
                    Views: ScopeView · SpectrumView · VectorView · MeterView
```

| Part | File | Notes |
|---|---|---|
| Processor | `Source/PluginProcessor.*` | Parameters (output gain, chain bypass), state save/restore, analysis tap. |
| Chain host | `Source/hosting/PluginChain.*` | Wraps `juce::AudioProcessorGraph` like JUCE's AudioPluginHost, but restricted to a serial chain. Rewires on every change; mono/stereo plugins are fanned in/out automatically. Reports chain latency to the DAW. |
| Analysis | `Source/dsp/AnalysisFifo.h`, `Source/dsp/Analyzer.*` | Audio thread only copies samples into a FIFO, so the real-time path stays cheap. All analysis runs on the UI thread and only while the editor is open. |
| UI | `Source/PluginEditor.*`, `Source/ui/*` | Header (modes, freeze, tap, theme, size), display area, FX chain strip. |
| Themes | `Source/ui/Theme.h` | A theme is a plain colour struct; a themed LookAndFeel styles JUCE widgets. Adding a theme is one entry in a list. |

## Themes

Each theme is a colour set plus rendering traits, so it reads as a piece of
hardware rather than a recolour. Names are generic on purpose (no real brand
names or trade dress).

| Era | Theme | Inspired by | Traits |
|---|---|---|---|
| 1970s | Lab Scope | Green P31 phosphor bench oscilloscope | Glow, CRT vignette, graticule ticks, mono lettering |
| 1970s | Studio VU | Backlit VU meters on a walnut console | Needle VU meters, bevels, warm paper face |
| 1980s | LED Rack | 31-band rack graphic analyser | LED-ladder bands and meters, rack screws |
| 1980s | VFD Deck | Hi-fi vacuum-fluorescent displays | Cyan segments with bloom |
| 1990s | Grey LCD | Bevelled grey rack gear with reflective LCD | Chunky half-res pixels, LCD segments |
| 1990s | Beige Desktop | Early desktop audio software | Grey 3D widgets, navy accents, pixelation |
| 2000s | Brushed Aqua | Brushed aluminium and blue LCD | Brushed chassis, gloss highlight |
| 2000s | Carbon Gloss | Piano-black plugin GUIs | Gloss, orange accents |
| Modern | Studio Dark / Studio Light | Current flat plugin UIs | Flat, quiet |
| Retro-future | Synthwave | 80s neon future | Perspective grid, heavy neon glow |
| Retro-future | Amber Terminal | Starship computer terminal | Amber raster, scanlines, vignette |
| Retro-future | Hologram | Sci-fi HUD | Hex lattice, scanlines, translucent cyan |

## DSP

- **Spectrum:** 4096-point FFT (Hann window) on mid (L+R)/2, scaled so a
  full-scale sine reads 0 dB. Log frequency axis 20 Hz–20 kHz, adjustable
  tilt (0/3/4.5/6 dB per octave, pivot 1 kHz), fast attack and smooth release,
  slow-decaying peak-hold trace. Dominant peak uses parabolic interpolation
  between bins for sub-bin frequency accuracy.
- **Scope:** 32k-sample history ring. Rising zero-crossing trigger on mid for
  a stable picture, timebase 1–200 ms (mouse wheel). Min/max per pixel column
  when zoomed out, so nothing is lost.
- **Meters:** sample peak with hold, 300 ms RMS, ITU-R BS.1770 K-weighted
  loudness (momentary 400 ms, short-term 3 s) with 100 ms blocks, Pearson
  correlation.

## Plugin hosting

- `AudioPluginFormatManager` with default formats (VST3 everywhere, AU on
  macOS). Plugins are found with JUCE's `PluginListComponent` scanner
  ("Plugins…" button). The list is saved to the user app-data folder and
  shared between instances.
- State of the chain (each plugin's description, bypass and its own state
  blob) is stored in the session, so a project reopens with the same chain.
- Known risk: a crashing plugin during scan or processing takes the DAW down,
  because it runs in-process. The dead-man's-pedal file skips crashers on the
  next scan. Out-of-process scanning is a later milestone.

## Milestones

1. **v0.1 (this build):** everything above, compiling on Linux as VST3 +
   Standalone.
2. **v0.2:** Windows/macOS builds via GitHub Actions, out-of-process
   scanning, drag-to-reorder slots, per-slot wet/dry.
3. **v0.3:** OpenGL rendering, spectrogram (waterfall) view, L/R and M/S
   spectrum overlays, second-input sidechain comparison.
4. **v1.0:** brand art and logo, presets, installer, code signing and
   notarisation, licence/activation, manual.

## Decisions needed

- Brand and product name (plugin codes `Ybrd`/`Scp1` change with it, and
  must stay fixed after release).
- Target OS for the first release (Windows, macOS, or both).
- Licensing: JUCE 8 is AGPLv3 or a paid commercial licence (free tier
  available under a revenue cap). Closed-source sales need the commercial
  licence. VST3 SDK is MIT-licensed since 3.8.
