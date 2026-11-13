// Renders PNG screenshots of the Scope editor for every theme and mode,
// feeding it a test signal. Usage: Snapshots <outDir>
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

static void fillSignal (AudioBuffer<float>& b, int64& pos, double sr)
{
    auto& rng = Random::getSystemRandom();
    for (int i = 0; i < b.getNumSamples(); ++i, ++pos)
    {
        const double t = (double) pos / sr;
        const float base = 0.35f * (float) std::sin (MathConstants<double>::twoPi * 110.0 * t)
                         + 0.18f * (float) std::sin (MathConstants<double>::twoPi * 440.0 * t)
                         + 0.08f * (float) std::sin (MathConstants<double>::twoPi * 2637.0 * t);
        const float noise = 0.02f * (rng.nextFloat() * 2.0f - 1.0f);
        b.setSample (0, i, base + noise);
        b.setSample (1, i, 0.8f * base + 0.1f * (float) std::sin (MathConstants<double>::twoPi * 660.0 * t) + noise);
    }
}

int main (int argc, char* argv[])
{
    ScopedJuceInitialiser_GUI gui;
    const File outDir (argc > 1 ? argv[1] : "shots");
    outDir.createDirectory();
    const double sr = 48000.0; const int bs = 512;

    ScopeAudioProcessor proc;
    proc.setPlayConfigDetails (2, 2, sr, bs);
    proc.prepareToPlay (sr, bs);

    struct Shot { int theme, mode, w, h; };
    std::vector<Shot> shots;
    const int numThemes = (int) scope::getThemes().size();
    for (int t = 0; t < numThemes; ++t) shots.push_back ({ t, 0, 1200, 740 });
    for (int t : { 1, 2, 10 })
        for (int m = 1; m < 5; ++m) shots.push_back ({ t, m, 900, 560 });

    int64 pos = 0;
    AudioBuffer<float> buf (2, bs); MidiBuffer midi;
    for (auto& s : shots)
    {
        proc.uiState.setProperty ("theme", s.theme, nullptr);
        proc.uiState.setProperty ("mode", s.mode, nullptr);
        proc.uiState.setProperty ("width", s.w, nullptr);
        proc.uiState.setProperty ("height", s.h, nullptr);
        auto ed = std::make_unique<ScopeAudioProcessorEditor> (proc);
        for (int k = 0; k < 120; ++k)
        {
            fillSignal (buf, pos, sr);
            proc.processBlock (buf, midi);
            if (k % 2 == 0) { Thread::sleep (16); ed->pumpAnalysis(); }
        }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        const auto& th = scope::getTheme (s.theme);
        auto f = outDir.getChildFile (String (s.theme).paddedLeft ('0', 2) + "_" + th.name.replace (" ", "") + "_mode" + String (s.mode) + ".png");
        f.deleteFile();
        FileOutputStream os (f);
        PNGImageFormat().writeImageToStream (img, os);
        std::cout << "wrote " << f.getFileName() << "\n";
    }
    proc.releaseResources();
    return 0;
}
