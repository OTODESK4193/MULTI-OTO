#pragma once
#include "PluginProcessor.h"
#include "GUI/MinimalUI.h"

class MultiOtoAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Button::Listener {
public:
    MultiOtoAudioProcessorEditor(MultiOtoAudioProcessor&);
    ~MultiOtoAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;

private:
    MultiOtoAudioProcessor& audioProcessor;
    MultiOtoLookAndFeel laf;

    juce::GroupComponent preDriveGroup, stage1Group, stage2Group, masterGroup;
    juce::TextButton s1AdvBtn{ "ADVANCED" }, s2AdvBtn{ "ADVANCED" };
    bool s1AdvOpen = false, s2AdvOpen = false;

    ArcKnob inGain, drive, oddBlend, evenBlend, totalOtt;
    ArcKnob xLow, xHigh;

    ArcKnob s1GainH, s1GainM, s1GainL, s1DepthH, s1DepthM, s1DepthL, s1Time;
    ArcKnob s1AtkH, s1AtkM, s1AtkL, s1RelH, s1RelM, s1RelL;

    ArcKnob s2GainH, s2GainM, s2GainL, s2DepthH, s2DepthM, s2DepthL, s2Time;
    ArcKnob s2AtkH, s2AtkM, s2AtkL, s2RelH, s2RelM, s2RelL;

    ArcKnob postHPF, postLPF, dryWet, outGain, limitCeil;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiOtoAudioProcessorEditor)
};