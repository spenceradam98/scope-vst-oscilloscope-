// Headless host test: loads the built Scope.vst3 like a DAW would, inserts
// TestHalfGain.vst3 into Scope's FX chain via saved state, and checks the
// audio coming out of Scope is halved.
// Usage: HostTest <Scope.vst3> <TestHalfGain.vst3>
#include <juce_audio_utils/juce_audio_utils.h>

using namespace juce;

static void fillSignal (AudioBuffer<float>& b, int64& pos, double sr)
{
    Random& rng = Random::getSystemRandom();
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

static MemoryBlock makeState (const PluginDescription* fx, int theme, int mode, int w, int h)
{
    XmlElement root ("SCOPE");
    auto* ui = root.createNewChildElement ("UI");
    ui->setAttribute ("theme", theme);
    ui->setAttribute ("mode", mode);
    ui->setAttribute ("width", w);
    ui->setAttribute ("height", h);
    auto* chain = root.createNewChildElement ("CHAIN");
    if (fx != nullptr)
        chain->createNewChildElement ("SLOT")->addChildElement (fx->createXml().release());
    MemoryBlock inner;
    AudioProcessor::copyXmlToBinary (root, inner);
    // A JUCE VST3 host expects its own wrapper around the component state.
    XmlElement wrapper ("VST3PluginState");
    wrapper.createNewChildElement ("IComponent")->addTextElement (inner.toBase64Encoding());
    MemoryBlock mb;
    AudioProcessor::copyXmlToBinary (wrapper, mb);
    return mb;
}

int main (int argc, char* argv[])
{
    if (argc < 3) { std::cerr << "usage: HostTest scope.vst3 fx.vst3\n"; return 2; }
    ScopedJuceInitialiser_GUI gui;
    const double sr = 48000.0; const int bs = 512;

    AudioPluginFormatManager fm;
    fm.addFormat (new VST3PluginFormat());
    auto* vst3 = fm.getFormat (0);

    OwnedArray<PluginDescription> scopeTypes, fxTypes;
    vst3->findAllTypesForFile (scopeTypes, argv[1]);
    vst3->findAllTypesForFile (fxTypes, argv[2]);
    if (scopeTypes.isEmpty() || fxTypes.isEmpty()) { std::cerr << "could not scan plugins\n"; return 1; }

    String err;
    auto scope = fm.createPluginInstance (*scopeTypes[0], sr, bs, err);
    if (! scope) { std::cerr << "load failed: " << err << "\n"; return 1; }
    scope->setPlayConfigDetails (2, 2, sr, bs);
    scope->prepareToPlay (sr, bs);
    std::cout << "Loaded " << scope->getName() << "\n";

    // --- Hosting test: chain with TestHalfGain must halve the signal.
    {
        auto st = makeState (fxTypes[0], 0, 0, 900, 560);
        scope->setStateInformation (st.getData(), (int) st.getSize());
        MessageManager::getInstance()->runDispatchLoopUntil (300);
        AudioBuffer<float> buf (2, bs); MidiBuffer midi; int64 pos = 0;
        double inSum = 0, outSum = 0;
        for (int k = 0; k < 40; ++k)
        {
            fillSignal (buf, pos, sr);
            inSum += buf.getRMSLevel (0, 0, bs);
            scope->processBlock (buf, midi);
            outSum += buf.getRMSLevel (0, 0, bs);
        }
        const double ratio = outSum / inSum;
        std::cout << "Chain gain ratio (expect 0.5): " << ratio << "\n";
        if (std::abs (ratio - 0.5) > 0.01) { std::cerr << "HOSTING TEST FAILED\n"; return 1; }
        std::cout << "HOSTING TEST PASSED\n";
    }

    scope->releaseResources();
    scope.reset();
    return 0;
}
