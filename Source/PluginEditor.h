#pragma once
#include "PluginProcessor.h"
#include "GUI/MinimalUI.h"
#include "GUI/TabHeader.h"
#include "GUI/MainPanel.h"

// ============================================================================
//  LIFT-X 準拠 ContentComponent パターン (3段構成 ＆ 超大型ノブデザイン)
//  内部は kBaseW (880px) × kBaseH (750px) の論理座標で動作
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
    static constexpr int kBaseH = 580;

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

    private:
        MultiOtoAudioProcessor& processor;

        TabHeader header;
        MainPanel mainPanel;
    } content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiOtoAudioProcessorEditor)
};