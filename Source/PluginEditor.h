#pragma once
#include "PluginProcessor.h"
#include "GUI/MinimalUI.h"
#include "GUI/TabHeader.h"
#include "GUI/MainPanel.h"
#include "GUI/PresetBrowser.h"
#include "GUI/ConfigPanel.h"
#include "GUI/ModPanel.h"

// ============================================================================
//  ContentComponent パターン
//  内部は kBaseW (880px) × kBaseH (620px) の論理座標で動作し、
//  リサイズ時はアフィン変換でスケールする。
// ============================================================================
class MultiOtoAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    MultiOtoAudioProcessorEditor (MultiOtoAudioProcessor&);
    ~MultiOtoAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int kBaseW = 880;
    static constexpr int kBaseH = 620;

    MultiOtoAudioProcessor& audioProcessor;
    MultiOtoLookAndFeel laf;

    juce::ComponentBoundsConstrainer constrainer;

    struct ContentComponent : public juce::Component
    {
        ContentComponent (MultiOtoAudioProcessor& proc, MultiOtoLookAndFeel& laf);
        ~ContentComponent() override;
        void paint (juce::Graphics&) override;
        void resized() override;

        void connectMeters();
        void wirePresetBrowser();
        void refreshPresetName();
        void applyThemeFromParam();

    private:
        void loadPresetWithBatch (const PresetRef& ref);

        MultiOtoAudioProcessor& processor;
        MultiOtoLookAndFeel& lookAndFeelRef;

        TabHeader header;
        MainPanel mainPanel;
        PresetBrowser presetBrowser;
        ConfigPanel configPanel;
        ModPanel modPanel;
    } content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiOtoAudioProcessorEditor)
};
