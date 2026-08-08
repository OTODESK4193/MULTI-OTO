#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/EngineCore.h"
#include "Presets/PresetData.h"

// ============================================================================
//  ContentComponent
// ============================================================================
MultiOtoAudioProcessorEditor::ContentComponent::ContentComponent (
    MultiOtoAudioProcessor& proc, MultiOtoLookAndFeel& laf)
    : processor (proc),
      mainPanel (proc.apvts, laf)
{
    addAndMakeVisible (header);
    addAndMakeVisible (mainPanel);

    presetBrowser.setVisible (false);
    addChildComponent (presetBrowser);

    mainPanel.bindApvts (proc.apvts);

    header.onPresetClicked = [this] { presetBrowser.setVisible (! presetBrowser.isVisible()); };

    wirePresetBrowser();
    refreshPresetName();

    // プリセット名が変わったらヘッダー表示を更新する
    juce::Component::SafePointer<ContentComponent> safeThis (this);
    processor.onPresetNameChanged = [safeThis]
    {
        if (safeThis != nullptr) safeThis->refreshPresetName();
    };
}

MultiOtoAudioProcessorEditor::ContentComponent::~ContentComponent()
{
    processor.onPresetNameChanged = nullptr;
}

void MultiOtoAudioProcessorEditor::ContentComponent::refreshPresetName()
{
    header.setPresetName (processor.getCurrentPresetName());
}

void MultiOtoAudioProcessorEditor::ContentComponent::wirePresetBrowser()
{
    presetBrowser.getCategories = []
    {
        // FACTORY とユーザーフォルダのカテゴリを統合して一覧にする
        auto cats = PresetData::getFactoryCategories();
        cats.addArray (MultiOtoAudioProcessor::getUserPresetCategories());
        cats.removeDuplicates (true);
        cats.sort (true);
        return cats;
    };

    presetBrowser.getPresetsForCategory = [] (juce::String category)
    {
        juce::Array<PresetRef> out;

        // --- 内蔵 FACTORY ---
        const auto& fac = PresetData::getFactoryPresets();
        for (int i = 0; i < (int) fac.size(); ++i)
        {
            const auto& p = fac[(size_t) i];
            if (category.isEmpty() || category.equalsIgnoreCase (p.category))
            {
                PresetRef r;
                r.name         = p.name;
                r.category     = p.category;
                r.description  = juce::String (p.description)
                               + "   [推奨 OTT: x" + juce::String (p.suggestedCount) + "]";
                r.isFactory    = true;
                r.factoryIndex = i;
                out.add (r);
            }
        }

        // --- ユーザープリセット (ディスク) ---
        const auto root = MultiOtoAudioProcessor::getPresetRootDirectory();
        if (root.isDirectory())
        {
            const auto searchDir = category.isEmpty() ? root : root.getChildFile (category);
            if (searchDir.isDirectory())
            {
                const auto wildcard = "*." + MultiOtoAudioProcessor::getPresetFileExtension();
                for (const auto& e : juce::RangedDirectoryIterator (searchDir, category.isEmpty(),
                                                                    wildcard, juce::File::findFiles))
                {
                    PresetRef r;
                    r.name        = e.getFile().getFileNameWithoutExtension();
                    r.category    = e.getFile().getParentDirectory().getFileName();
                    r.description = "User preset";
                    r.isFactory   = false;
                    r.file        = e.getFile();
                    out.add (r);
                }
            }
        }

        // 表示順を安定させる (FACTORY が先、その中は名前順)
        struct Sorter {
            static int compareElements (const PresetRef& a, const PresetRef& b)
            {
                if (a.isFactory != b.isFactory) return a.isFactory ? -1 : 1;
                return a.name.compareIgnoreCase (b.name);
            }
        };
        Sorter sorter;
        out.sort (sorter);
        return out;
    };

    presetBrowser.onPresetChosen = [this] (PresetRef ref)
    {
        if (ref.isFactory)
        {
            processor.loadFactoryPreset (ref.factoryIndex);
        }
        else
        {
            juce::String error;
            if (! processor.loadPresetFile (ref.file, error))
            {
                presetBrowser.showMessage ("Load Failed", error);
                return;
            }
        }

        refreshPresetName();
        presetBrowser.setVisible (false);
        repaint();
    };

    presetBrowser.onSaveRequested = [this] (juce::String category, juce::String name)
    {
        juce::String error;
        if (! processor.savePreset (category, name, error))
            presetBrowser.showMessage ("Save Failed", error);
        else
            refreshPresetName();
    };

    presetBrowser.onPresetDeleteRequested = [] (juce::File file) -> bool
    {
        // 安全策: プリセットルート配下の .motopreset 以外は絶対に消さない
        const auto root = MultiOtoAudioProcessor::getPresetRootDirectory();

        if (! file.existsAsFile()) return false;
        if (! file.hasFileExtension (MultiOtoAudioProcessor::getPresetFileExtension())) return false;
        if (! file.isAChildOf (root)) return false;

        if (! file.moveToTrash())
            return file.deleteFile();   // ゴミ箱が使えない環境では直接削除

        return true;
    };

    presetBrowser.onInitConfirmed = [this]
    {
        processor.resetToInit();
        refreshPresetName();
        presetBrowser.setVisible (false);
        repaint();
    };
}

void MultiOtoAudioProcessorEditor::ContentComponent::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (
        MOColors::bg, 0, 0,
        MOColors::bg.darker (0.15f), 0, (float) getHeight(), false));
    g.fillAll();
}

void MultiOtoAudioProcessorEditor::ContentComponent::resized()
{
    auto area = getLocalBounds();

    header.setBounds (area.removeFromTop (32));
    mainPanel.setBounds (area.reduced (4, 2));

    // ブラウザはヘッダーを除いた全面をオーバーレイする
    presetBrowser.setBounds (getLocalBounds().withTrimmedTop (32));
}

void MultiOtoAudioProcessorEditor::ContentComponent::connectMeters()
{
    auto* engine = processor.getEngineCore();
    if (engine == nullptr) return;

    mainPanel.setMeters (&engine->s1Meter, &engine->s2Meter);
}

// ============================================================================
//  PluginEditor
// ============================================================================
MultiOtoAudioProcessorEditor::MultiOtoAudioProcessorEditor (MultiOtoAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      content (p, laf)
{
    setOpaque (true);
    setLookAndFeel (&laf);

    constrainer.setFixedAspectRatio ((double) kBaseW / (double) kBaseH);
    constrainer.setMinimumSize (kBaseW / 2, kBaseH / 2);
    constrainer.setMaximumSize (kBaseW * 2, kBaseH * 2);
    setConstrainer (&constrainer);
    setResizable (true, true);

    addAndMakeVisible (content);

    content.connectMeters();

    setSize (kBaseW, kBaseH);
}

MultiOtoAudioProcessorEditor::~MultiOtoAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void MultiOtoAudioProcessorEditor::paint (juce::Graphics&)
{
}

void MultiOtoAudioProcessorEditor::resized()
{
    const float scale = (float) getWidth() / (float) kBaseW;
    content.setTransform (juce::AffineTransform::scale (scale));
    content.setBounds (0, 0, kBaseW, kBaseH);
}
