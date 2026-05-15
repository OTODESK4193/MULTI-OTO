#pragma once
#include "PluginProcessor.h"
#include "GUI/MinimalUI.h"

class MultiOtoAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Button::Listener {
public:
    MultiOtoAudioProcessorEditor(MultiOtoAudioProcessor&);
    ~MultiOtoAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void buttonClicked(juce::Button* b) override;

private:
    MultiOtoAudioProcessor& audioProcessor;
    MultiOtoLookAndFeel laf;

    juce::GroupComponent preDriveGroup, stage1Group, stage2Group, masterGroup;

    juce::TextButton preDriveBtn{ "ON" };
    juce::TextButton stage1Btn{ "ON" };
    juce::TextButton stage2Btn{ "ON" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> preDriveAt, s1At, s2At;

    juce::TextButton s1AdvBtn{ "ADVANCED" };
    juce::TextButton s2AdvBtn{ "ADVANCED" };
    bool s1AdvOpen = false, s2AdvOpen = false;

    juce::ComboBox totalOttBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> totalOttAttachment;
    juce::Label totalOttLabel;

    ArcKnob inGain, drive, oddBlend, evenBlend;
    ArcKnob xLow, xHigh;

    ArcKnob s1GainL, s1GainM, s1GainH;
    ArcKnob s1DepthL, s1DepthM, s1DepthH;
    ArcKnob s1Time, s1Mix;
    ArcKnob s1AtkL, s1AtkM, s1AtkH;
    ArcKnob s1RelL, s1RelM, s1RelH;

    ArcKnob s2GainL, s2GainM, s2GainH;
    ArcKnob s2DepthL, s2DepthM, s2DepthH;
    ArcKnob s2Time, s2Mix;
    ArcKnob s2AtkL, s2AtkM, s2AtkH;
    ArcKnob s2RelL, s2RelM, s2RelH;

    ArcKnob postHPF, postLPF, dryWet, outGain, limitCeil;

    juce::ComboBox phaseModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> phaseModeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiOtoAudioProcessorEditor)
};