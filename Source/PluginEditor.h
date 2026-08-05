#pragma once
#include "PluginProcessor.h"
#include "GUI/MinimalUI.h"
#include "GUI/TabHeader.h"
#include "GUI/MainPanel.h"
#include "GUI/StagePanel.h"

// ============================================================================
//  LIFT-X 準拠 ContentComponent パターン (全3タブ構成)
//  内部は kBaseW (860px) × kBaseH (640px) の論理座標で動作
// ============================================================================
class MultiOtoAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    MultiOtoAudioProcessorEditor (MultiOtoAudioProcessor&);
    ~MultiOtoAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int kBaseW = 860;
    static constexpr int kBaseH = 640;

    MultiOtoAudioProcessor& audioProcessor;
    MultiOtoLookAndFeel laf;

    juce::ComponentBoundsConstrainer constrainer;

    struct ContentComponent : public juce::Component
    {
        ContentComponent (MultiOtoAudioProcessor& proc, MultiOtoLookAndFeel& laf);
        ~ContentComponent() override;
        void paint (juce::Graphics&) override;
        void resized() override;

        void setActiveTab (TabHeader::Tab t);
        void connectMeters();

    private:
        MultiOtoAudioProcessor& processor;

        TabHeader header;
        MainPanel  mainPanel;
        StagePanel stage1Panel;
        StagePanel stage2Panel;
    } content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiOtoAudioProcessorEditor)
};