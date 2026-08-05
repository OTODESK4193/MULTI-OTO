#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ColorPalette.h"
#include "MinimalUI.h"
#include "DynVisual.h"

struct StageMeter;

// ============================================================================
//  StagePanel — STAGE 1 / STAGE 2 共用パネル
//  上部: DynVisual (大画面・インタラクティブドラッグ対応)
//  下部: GAIN / UPWARD / DOWNWARD / DEPTH / ATK / REL × 3バンド + TIME / MIX
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
        dynVis.bindStageParameters (apvts, stageNum);
        addAndMakeVisible (dynVis);

        // GAIN
        gainL.build (apvts, "s" + st + "_gain_l", "LOW G",  this, laf);
        gainM.build (apvts, "s" + st + "_gain_m", "MID G",  this, laf);
        gainH.build (apvts, "s" + st + "_gain_h", "HI G",   this, laf);

        // UPWARD (弱音引き上げ %)
        upL.build (apvts, "s" + st + "_up_l", "LOW UP", this, laf);
        upM.build (apvts, "s" + st + "_up_m", "MID UP", this, laf);
        upH.build (apvts, "s" + st + "_up_h", "HI UP",  this, laf);

        // DOWNWARD (大音量圧縮 %)
        dnL.build (apvts, "s" + st + "_down_l", "LOW DN", this, laf);
        dnM.build (apvts, "s" + st + "_down_m", "MID DN", this, laf);
        dnH.build (apvts, "s" + st + "_down_h", "HI DN",  this, laf);

        // DEPTH
        depthL.build (apvts, "s" + st + "_depth_l", "LOW D", this, laf);
        depthM.build (apvts, "s" + st + "_depth_m", "MID D", this, laf);
        depthH.build (apvts, "s" + st + "_depth_h", "HI D",  this, laf);

        // TIME, MIX
        time.build (apvts, "s" + st + "_time", "TIME", this, laf);
        mix.build  (apvts, "s" + st + "_mix",  "MIX",  this, laf);

        // ATTACK
        atkL.build (apvts, "s" + st + "_atk_l", "ATK L", this, laf);
        atkM.build (apvts, "s" + st + "_atk_m", "ATK M", this, laf);
        atkH.build (apvts, "s" + st + "_atk_h", "ATK H", this, laf);

        // RELEASE
        relL.build (apvts, "s" + st + "_rel_l", "REL L", this, laf);
        relM.build (apvts, "s" + st + "_rel_m", "REL M", this, laf);
        relH.build (apvts, "s" + st + "_rel_h", "REL H", this, laf);
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
        auto area = getLocalBounds().reduced (12, 8);

        // 1. 上部 DynVisual 描画エリア (高さ 190px)
        dynVis.setBounds (area.removeFromTop (190));

        area.removeFromTop (8);

        // 2. 下部 コントロールエリア
        auto ctrlRow = area.removeFromTop (26);
        onBtn.setBounds (ctrlRow.removeFromLeft (80).withSizeKeepingCentre (75, 24));

        area.removeFromTop (6);

        int kS   = 76;
        int gapX = 14;

        // 行1: GAIN L/M/H | UPWARD L/M/H | TIME, MIX (8ノブ)
        auto row1 = area.removeFromTop (kS);
        gainL.setBounds (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX);
        gainM.setBounds (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX);
        gainH.setBounds (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX + 15);

        upL.setBounds   (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX);
        upM.setBounds   (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX);
        upH.setBounds   (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX + 15);

        time.setBounds  (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX);
        mix.setBounds   (row1.removeFromLeft (kS));

        area.removeFromTop (10);

        // 行2: DOWNWARD L/M/H | ATTACK L/M/H | RELEASE L/M/H (8ノブ)
        auto row2 = area.removeFromTop (kS);
        dnL.setBounds  (row2.removeFromLeft (kS)); row2.removeFromLeft (gapX);
        dnM.setBounds  (row2.removeFromLeft (kS)); row2.removeFromLeft (gapX);
        dnH.setBounds  (row2.removeFromLeft (kS)); row2.removeFromLeft (gapX + 15);

        atkL.setBounds (row2.removeFromLeft (kS)); row2.removeFromLeft (gapX);
        atkM.setBounds (row2.removeFromLeft (kS)); row2.removeFromLeft (gapX);
        atkH.setBounds (row2.removeFromLeft (kS)); row2.removeFromLeft (gapX + 15);

        relL.setBounds (row2.removeFromLeft (kS)); row2.removeFromLeft (gapX);
        relM.setBounds (row2.removeFromLeft (kS));
    }

private:
    int stage;

    juce::TextButton onBtn { "STAGE ON" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAt;

    DynVisualComponent dynVis;

    ArcKnob gainL, gainM, gainH;
    ArcKnob upL, upM, upH;
    ArcKnob dnL, dnM, dnH;
    ArcKnob depthL, depthM, depthH;
    ArcKnob time, mix;
    ArcKnob atkL, atkM, atkH;
    ArcKnob relL, relM, relH;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StagePanel)
};
