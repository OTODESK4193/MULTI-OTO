#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ColorPalette.h"
#include "MinimalUI.h"
#include "DynVisual.h"

struct StageMeter;

// ============================================================================
//  StagePanel — STAGE 1 / STAGE 2 共用パネル
//  全タブ共通の上部 DynVisual 描画エリア (高さ 200px)
//  下部: GAIN/DEPTH/ATK/REL (3バンド) + TIME/MIX 大きめノブ
// ============================================================================
class StagePanel : public juce::Component
{
public:
    StagePanel (juce::AudioProcessorValueTreeState& apvts, int stageNum,
                MultiOtoLookAndFeel& laf)
        : stage (stageNum)
    {
        juce::String st = juce::String (stageNum);

        onBtn.setClickingTogglesState (true);
        onBtn.setColour (juce::TextButton::buttonColourId,   MOColors::knobTrack);
        onBtn.setColour (juce::TextButton::buttonOnColourId, stageNum == 1 ? MOColors::peach : MOColors::babyBlue);
        onBtn.setColour (juce::TextButton::textColourOffId,  MOColors::textDim);
        onBtn.setColour (juce::TextButton::textColourOnId,   MOColors::bg);
        addAndMakeVisible (onBtn);
        onAt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            apvts, "s" + st + "_on", onBtn);

        dynVis.setTitle ("STAGE " + st + " / 3-BAND OTT VISUALIZER");
        addAndMakeVisible (dynVis);

        // GAIN
        gainL.build (apvts, "s" + st + "_gain_l", "LOW GAIN",  this, laf);
        gainM.build (apvts, "s" + st + "_gain_m", "MID GAIN",  this, laf);
        gainH.build (apvts, "s" + st + "_gain_h", "HI GAIN",   this, laf);

        // DEPTH
        depthL.build (apvts, "s" + st + "_depth_l", "LOW DEPTH", this, laf);
        depthM.build (apvts, "s" + st + "_depth_m", "MID DEPTH", this, laf);
        depthH.build (apvts, "s" + st + "_depth_h", "HI DEPTH",  this, laf);

        // TIME, MIX
        time.build (apvts, "s" + st + "_time", "TIME", this, laf);
        mix.build  (apvts, "s" + st + "_mix",  "MIX",  this, laf);

        // ATTACK
        atkL.build (apvts, "s" + st + "_atk_l", "ATTACK L", this, laf);
        atkM.build (apvts, "s" + st + "_atk_m", "ATTACK M", this, laf);
        atkH.build (apvts, "s" + st + "_atk_h", "ATTACK H", this, laf);

        // RELEASE
        relL.build (apvts, "s" + st + "_rel_l", "RELEASE L", this, laf);
        relM.build (apvts, "s" + st + "_rel_m", "RELEASE M", this, laf);
        relH.build (apvts, "s" + st + "_rel_h", "RELEASE H", this, laf);
    }

    void setMeter (const StageMeter* m, std::atomic<float>* xLo, std::atomic<float>* xHi)
    {
        dynVis.setMeter (m);
        dynVis.setCrossoverFreqs (xLo, xHi);
    }

    void paint (juce::Graphics& g) override
    {
        MOColors::paintPanel (g, getLocalBounds());
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (12, 10);

        // 1. 全タブ共通 上部 DynVisual 描画エリア (高さ 200px)
        dynVis.setBounds (area.removeFromTop (200));

        area.removeFromTop (12);

        // 2. 下部 コントロールエリア
        // ON ボタン
        auto ctrlRow = area.removeFromTop (28);
        onBtn.setBounds (ctrlRow.removeFromLeft (80).withSizeKeepingCentre (75, 26));

        area.removeFromTop (10);

        int kS   = 82;   // 大きめノブ (82px)
        int gapX = 18;

        // 行1: GAIN (LOW, MID, HI) | DEPTH (LOW, MID, HI) | TIME, MIX (全8ノブ横並び配置)
        auto row1 = area.removeFromTop (kS);
        gainL.setBounds  (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX);
        gainM.setBounds  (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX);
        gainH.setBounds  (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX + 15);

        depthL.setBounds (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX);
        depthM.setBounds (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX);
        depthH.setBounds (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX + 15);

        time.setBounds   (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX);
        mix.setBounds    (row1.removeFromLeft (kS));

        area.removeFromTop (15);

        // 行2: ATTACK (LOW, MID, HI) | RELEASE (LOW, MID, HI)
        auto row2 = area.removeFromTop (kS);
        atkL.setBounds (row2.removeFromLeft (kS)); row2.removeFromLeft (gapX);
        atkM.setBounds (row2.removeFromLeft (kS)); row2.removeFromLeft (gapX);
        atkH.setBounds (row2.removeFromLeft (kS)); row2.removeFromLeft (gapX + 15);

        relL.setBounds (row2.removeFromLeft (kS)); row2.removeFromLeft (gapX);
        relM.setBounds (row2.removeFromLeft (kS)); row2.removeFromLeft (gapX);
        relH.setBounds (row2.removeFromLeft (kS));
    }

private:
    int stage;

    juce::TextButton onBtn { "STAGE ON" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAt;

    DynVisualComponent dynVis;

    ArcKnob gainL, gainM, gainH;
    ArcKnob depthL, depthM, depthH;
    ArcKnob time, mix;
    ArcKnob atkL, atkM, atkH;
    ArcKnob relL, relM, relH;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StagePanel)
};
