#include "ChainPanel.h"

namespace scope
{
using namespace juce;

//==============================================================================
class ChainPanel::PluginWindow : public DocumentWindow
{
public:
    PluginWindow (AudioProcessor& p, std::function<void (AudioProcessor*)> onClose)
        : DocumentWindow (p.getName(), Colours::darkgrey, DocumentWindow::closeButton), processor (p), closed (std::move (onClose))
    {
        setUsingNativeTitleBar (true);
        if (auto* ed = p.createEditorIfNeeded())
            setContentNonOwned (ed, true);
        else
            setContentOwned (new GenericAudioProcessorEditor (p), true);
        setResizable (getContentComponent() != nullptr && dynamic_cast<AudioProcessorEditor*> (getContentComponent())->isResizable(), false);
        centreWithSize (getWidth(), getHeight());
        setVisible (true);
        setAlwaysOnTop (true);
    }

    ~PluginWindow() override
    {
        if (auto* ed = dynamic_cast<AudioProcessorEditor*> (getContentComponent()))
        {
            processor.editorBeingDeleted (ed);
            clearContentComponent();
            delete ed;
        }
    }

    void closeButtonPressed() override { MessageManager::callAsync ([c = closed, p = &processor] { c (p); }); }

    AudioProcessor& processor;

private:
    std::function<void (AudioProcessor*)> closed;
};

//==============================================================================
class ChainPanel::ScannerWindow : public DocumentWindow
{
public:
    ScannerWindow (PluginChain& c, std::function<void()> onClose)
        : DocumentWindow ("Plugins", Colours::darkgrey, DocumentWindow::closeButton), chain (c), closed (std::move (onClose))
    {
        setUsingNativeTitleBar (true);
        auto* list = new PluginListComponent (chain.formatManager, chain.knownPlugins, chain.getDeadMansPedalFile(), nullptr, false);
        list->setSize (640, 480);
        setContentOwned (list, true);
        setResizable (true, false);
        centreWithSize (640, 480);
        setVisible (true);
        setAlwaysOnTop (true);
    }

    void closeButtonPressed() override
    {
        chain.saveKnownPlugins();
        MessageManager::callAsync (closed);
    }

private:
    PluginChain& chain;
    std::function<void()> closed;
};

//==============================================================================
ChainPanel::ChainPanel (PluginChain& c, AudioProcessorValueTreeState& apvts)
    : chain (c),
      bypassAttachment (apvts, "chainBypass", bypassButton),
      gainAttachment (apvts, "outputGain", outGain)
{
    title.setJustificationType (Justification::centredLeft);
    addAndMakeVisible (title);
    bypassButton.setClickingTogglesState (true);
    bypassButton.setTooltip ("Bypass the whole FX chain");
    addAndMakeVisible (bypassButton);
    addButton.onClick = [this] { showAddMenu(); };
    addButton.setTooltip ("Insert a plugin at the end of the chain");
    addAndMakeVisible (addButton);
    scanButton.onClick = [this] { openScanner(); };
    scanButton.setTooltip ("Scan for and manage installed plugins");
    addAndMakeVisible (scanButton);
    outGain.setTextValueSuffix (" dB");
    outGain.setTextBoxStyle (Slider::TextBoxRight, false, 72, 20);
    outGain.setTooltip ("Output gain after the FX chain");
    addAndMakeVisible (outGain);
    addAndMakeVisible (outLabel);
    chain.addChangeListener (this);
    rebuild();
}

ChainPanel::~ChainPanel()
{
    chain.removeChangeListener (this);
    closeAllWindows();
}

void ChainPanel::closeAllWindows()
{
    windows.clear();
    if (scanner != nullptr)
    {
        chain.saveKnownPlugins();
        scanner.reset();
    }
}

void ChainPanel::changeListenerCallback (ChangeBroadcaster*) { rebuild(); }

void ChainPanel::rebuild()
{
    // Close windows whose plugin is gone
    windows.erase (std::remove_if (windows.begin(), windows.end(), [this] (auto& w)
    {
        for (int i = 0; i < chain.size(); ++i)
            if (chain.getPlugin (i) == &w->processor)
                return false;
        return true;
    }), windows.end());

    slotButtons.clear();
    for (int i = 0; i < chain.size(); ++i)
    {
        auto* p = chain.getPlugin (i);
        auto* b = slotButtons.add (new TextButton (String (i + 1) + ". " + (p ? p->getName() : String ("?"))));
        b->setTooltip ("Click to open, right-click for options");
        b->setAlpha (chain.isBypassed (i) ? 0.45f : 1.0f);
        b->onClick = [this, i]
        {
            if (std::exchange (suppressClick, false)) return;
            openEditor (i);
        };
        b->addMouseListener (this, false);
        addAndMakeVisible (b);
    }
    resized();
    repaint();
}

void ChainPanel::mouseDown (const MouseEvent& e)
{
    const int idx = slotButtons.indexOf (dynamic_cast<TextButton*> (e.eventComponent));
    if (idx >= 0 && e.mods.isPopupMenu())
    {
        suppressClick = true;
        showSlotMenu (idx);
    }
}

void ChainPanel::showAddMenu()
{
    auto types = chain.knownPlugins.getTypes();
    PopupMenu m;
    if (types.isEmpty())
    {
        m.addItem ("No plugins found yet. Scan for plugins...", [this] { openScanner(); });
    }
    else
    {
        KnownPluginList::addToMenu (m, types, KnownPluginList::sortByManufacturer);
        m.addSeparator();
        m.addItem ("Scan for plugins...", [this] { openScanner(); });
    }

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&addButton), [this, types] (int result)
    {
        const int idx = KnownPluginList::getIndexChosenByMenu (types, result);
        if (! isPositiveAndBelow (idx, types.size()))
            return;
        String error;
        if (! chain.addPlugin (types.getReference (idx), error))
            AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, "Could not load plugin", error);
        else
            openEditor (chain.size() - 1);
    });
}

void ChainPanel::showSlotMenu (int index)
{
    PopupMenu m;
    m.addItem ("Open editor", [this, index] { openEditor (index); });
    m.addItem ("Bypass", true, chain.isBypassed (index), [this, index] { chain.setBypassed (index, ! chain.isBypassed (index)); });
    m.addSeparator();
    m.addItem ("Move left", index > 0, false, [this, index] { chain.movePlugin (index, index - 1); });
    m.addItem ("Move right", index < chain.size() - 1, false, [this, index] { chain.movePlugin (index, index + 1); });
    m.addSeparator();
    m.addItem ("Remove", [this, index]
    {
        if (auto* p = chain.getPlugin (index)) closeEditor (p);
        chain.removePlugin (index);
    });
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (slotButtons[index]));
}

void ChainPanel::openEditor (int index)
{
    auto* p = chain.getPlugin (index);
    if (p == nullptr)
        return;
    for (auto& w : windows)
        if (&w->processor == p) { w->toFront (true); return; }
    windows.push_back (std::make_unique<PluginWindow> (*p, [this] (AudioProcessor* proc) { closeEditor (proc); }));
}

void ChainPanel::closeEditor (AudioProcessor* p)
{
    windows.erase (std::remove_if (windows.begin(), windows.end(), [p] (auto& w) { return &w->processor == p; }), windows.end());
}

void ChainPanel::openScanner()
{
    if (scanner != nullptr) { scanner->toFront (true); return; }
    scanner = std::make_unique<ScannerWindow> (chain, [this] { scanner.reset(); });
}

void ChainPanel::resized()
{
    auto r = getLocalBounds().reduced (8, 6);
    title.setFont (theme->font (12.0f, true));
    title.setBounds (r.removeFromLeft (72));
    bypassButton.setBounds (r.removeFromLeft (70)); r.removeFromLeft (8);

    auto right = r.removeFromRight (jlimit (200, 320, r.getWidth() / 3));
    scanButton.setBounds (right.removeFromRight (86)); right.removeFromRight (8);
    outLabel.setBounds (right.removeFromLeft (32));
    outGain.setBounds (right);
    r.removeFromRight (8);

    const int addW = 80;
    auto slotsArea = r.withTrimmedRight (addW + 6);
    const int n = slotButtons.size();
    const int w = n > 0 ? jmin (150, (slotsArea.getWidth() - (n - 1) * 4) / n) : 0;
    for (auto* b : slotButtons)
    {
        b->setBounds (slotsArea.removeFromLeft (w));
        slotsArea.removeFromLeft (4);
    }
    addButton.setBounds (slotsArea.getX() + 2, r.getY(), addW, r.getHeight());
}

void ChainPanel::paint (Graphics& g)
{
    g.setColour (theme->screenEdge.withAlpha (0.35f));
    g.fillRect (getLocalBounds().removeFromTop (1));
}
} // namespace scope
