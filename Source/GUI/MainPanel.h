#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ColorPalette.h"
#include "MinimalUI.h"
#include "DynVisual.h"

struct StageMeter;

// ============================================================================
//  MainPanel — 1画面統合パネル (高視認性大型ノブ ＆ 機能的上下対レイアウト)
//  上部: DynVisual (Stage1 & Stage2 横並び、タイトルボタンでStage選択)
//  中段: PRE-DRIVE & CROSSOVER + MASTER CONTROLS
//  下段: STAGE CONTROLS
//        ・GAIN (3)
//        ・UPWARD / DOWNWARD (上下対 3×2)
//        ・ATTACK / RELEASE (上下対 3×2, REL Hも完全描画)
//        ・GLOBAL (TIME, MIX)
// ============================================================================
class MainPanel : public juce::Component
{
public:
    MainPanel (juce::AudioProcessorValueTreeState& apvts, MultiOtoLookAndFeel& laf)
    {
        auto setupBtn = [&] (juce::TextButton& b, const char* pID,
                             std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& a) {
            b.setClickingTogglesState (true);
            b.setColour (juce::TextButton::buttonColourId,    MOColors::knobTrack);
            b.setColour (juce::TextButton::buttonOnColourId,  MOColors::accent);
            b.setColour (juce::TextButton::textColourOffId,   MOColors::textDim);
            b.setColour (juce::TextButton::textColourOnId,    MOColors::bg);
            addAndMakeVisible (b);
            a = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, pID, b);
        };

        setupBtn (preDriveBtn, "predrive_on", preDriveAt);

        totalOttBox.addItemList ({ "2","4","8","16","32","64","128" }, 1);
        totalOttBox.setLookAndFeel (&laf);
        addAndMakeVisible (totalOttBox);
        totalOttAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, "total_ott", totalOttBox);

        totalOttLabel.setText ("COUNT", juce::dontSendNotification);
        totalOttLabel.setColour (juce::Label::textColourId, MOColors::textDim);
        totalOttLabel.setJustificationType (juce::Justification::centred);
        totalOttLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        addAndMakeVisible (totalOttLabel);

        // PRE-DRIVE ノブ
        inGain.build    (apvts, "in_gain",    "IN GAIN",  this, laf);
        drive.build     (apvts, "drive",      "DRIVE",    this, laf);
        oddBlend.build  (apvts, "odd_blend",  "ODD",      this, laf);
        evenBlend.build (apvts, "even_blend", "EVEN",     this, laf);
        xLow.build      (apvts, "xover_low",  "LOW X",    this, laf);
        xHigh.build     (apvts, "xover_high", "HIGH X",   this, laf);

        // MASTER ノブ
        postHPF.build   (apvts, "post_hpf",   "HPF",      this, laf);
        postLPF.build   (apvts, "post_lpf",   "LPF",      this, laf);
        dryWet.build    (apvts, "dry_wet",     "DRY/WET",  this, laf);
        outGain.build   (apvts, "out_gain",    "OUT GAIN", this, laf);
        limitCeil.build (apvts, "limit_ceil",  "CEILING",  this, laf);

        phaseModeBox.addItemList ({ "COLOR PHASE", "ALIGN PHASE" }, 1);
        phaseModeBox.setLookAndFeel (&laf);
        phaseModeBox.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (phaseModeBox);
        phaseModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, "phase_mode", phaseModeBox);

        // DynVisuals
        dynS1.setTitle ("STAGE 1 (SELECT)");
        dynS2.setTitle ("STAGE 2 (SELECT)");
        dynS1.setSelected (true);
        dynS2.setSelected (false);

        dynS1.onStageSelected = [this] (int) { selectStage (1); };
        dynS2.onStageSelected = [this] (int) { selectStage (2); };

        addAndMakeVisible (dynS1);
        addAndMakeVisible (dynS2);

        // Stage ON ボタン
        setupBtn (s1OnBtn, "s1_on", s1OnAt);
        setupBtn (s2OnBtn, "s2_on", s2OnAt);
        s1OnBtn.setButtonText ("STAGE 1 ON");
        s2OnBtn.setButtonText ("STAGE 2 ON");

        // Stage 1/2 ノブ構築
        auto buildStageKnobs = [&] (int s, ArcKnob* gn, ArcKnob* up, ArcKnob* dn,
                                    ArcKnob& tm, ArcKnob& mx, ArcKnob* ak, ArcKnob* rl) {
            juce::String st = juce::String (s);
            gn[0].build (apvts, "s" + st + "_gain_l", "LOW G",  this, laf);
            gn[1].build (apvts, "s" + st + "_gain_m", "MID G",  this, laf);
            gn[2].build (apvts, "s" + st + "_gain_h", "HI G",   this, laf);

            up[0].build (apvts, "s" + st + "_up_l", "LOW UP", this, laf);
            up[1].build (apvts, "s" + st + "_up_m", "MID UP", this, laf);
            up[2].build (apvts, "s" + st + "_up_h", "HI UP",  this, laf);

            dn[0].build (apvts, "s" + st + "_down_l", "LOW DN", this, laf);
            dn[1].build (apvts, "s" + st + "_down_m", "MID DN", this, laf);
            dn[2].build (apvts, "s" + st + "_down_h", "HI DN",  this, laf);

            tm.build (apvts, "s" + st + "_time", "TIME", this, laf);
            mx.build (apvts, "s" + st + "_mix",  "MIX",  this, laf);

            ak[0].build (apvts, "s" + st + "_atk_l", "ATK L", this, laf);
            ak[1].build (apvts, "s" + st + "_atk_m", "ATK M", this, laf);
            ak[2].build (apvts, "s" + st + "_atk_h", "ATK H", this, laf);

            rl[0].build (apvts, "s" + st + "_rel_l", "REL L", this, laf);
            rl[1].build (apvts, "s" + st + "_rel_m", "REL M", this, laf);
            rl[2].build (apvts, "s" + st + "_rel_h", "REL H", this, laf);
        };

        buildStageKnobs (1, s1Gain, s1Up, s1Dn, s1Time, s1Mix, s1Atk, s1Rel);
        buildStageKnobs (2, s2Gain, s2Up, s2Dn, s2Time, s2Mix, s2Atk, s2Rel);

        // セクションラベル
        preLabel.setText ("PRE-DRIVE & CROSSOVER", juce::dontSendNotification);
        masterLabel.setText ("MASTER CONTROLS", juce::dontSendNotification);
        stageLabel.setText ("STAGE 1 CONTROLS", juce::dontSendNotification);

        for (auto* lbl : { &preLabel, &masterLabel, &stageLabel })
        {
            lbl->setColour (juce::Label::textColourId, MOColors::textDim);
            lbl->setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
            addAndMakeVisible (lbl);
        }

        updateStageVisibility();
    }

    ~MainPanel() override
    {
        totalOttBox.setLookAndFeel (nullptr);
        phaseModeBox.setLookAndFeel (nullptr);
    }

    void selectStage (int s)
    {
        activeStage = s;
        dynS1.setSelected (s == 1);
        dynS2.setSelected (s == 2);
        stageLabel.setText ("STAGE " + juce::String (s) + " CONTROLS", juce::dontSendNotification);
        stageLabel.setColour (juce::Label::textColourId, s == 1 ? MOColors::peach : MOColors::babyBlue);
        updateStageVisibility();
        resized();
    }

    void setMeters (const StageMeter* s1, const StageMeter* s2,
                    std::atomic<float>* xLo, std::atomic<float>* xHi)
    {
        dynS1.setMeter (s1);
        dynS2.setMeter (s2);
        dynS1.setCrossoverFreqs (xLo, xHi);
        dynS2.setCrossoverFreqs (xLo, xHi);
    }

    void bindApvts (juce::AudioProcessorValueTreeState& apvts)
    {
        dynS1.bindStageParameters (apvts, 1);
        dynS2.bindStageParameters (apvts, 2);
    }

    void paint (juce::Graphics& g) override
    {
        MOColors::paintPanel (g, getLocalBounds());
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (12, 8);

        // 1. 上部 DynVisual (高さ 190px)
        auto dynArea = area.removeFromTop (190);
        int halfW = (dynArea.getWidth() - 10) / 2;
        dynS1.setBounds (dynArea.removeFromLeft (halfW));
        dynArea.removeFromLeft (10);
        dynS2.setBounds (dynArea.removeFromLeft (halfW));

        area.removeFromTop (8);

        int midKnobW = 76;
        int midGapX  = 16;

        // 2. 中段 PRE-DRIVE & MASTER (高さ 150px)
        preLabel.setBounds (area.removeFromTop (16));
        area.removeFromTop (2);

        auto preRow = area.removeFromTop (60);
        inGain.setBounds    (preRow.removeFromLeft (midKnobW)); preRow.removeFromLeft (midGapX);
        drive.setBounds     (preRow.removeFromLeft (midKnobW)); preRow.removeFromLeft (midGapX);
        oddBlend.setBounds  (preRow.removeFromLeft (midKnobW)); preRow.removeFromLeft (midGapX);
        evenBlend.setBounds (preRow.removeFromLeft (midKnobW)); preRow.removeFromLeft (midGapX + 12);

        auto onCell = preRow.removeFromLeft (midKnobW);
        preDriveBtn.setBounds (onCell.withSizeKeepingCentre (74, 26));
        preRow.removeFromLeft (midGapX);

        auto countCell = preRow.removeFromLeft (midKnobW);
        totalOttBox.setBounds (countCell.withSizeKeepingCentre (74, 24).translated (0, -5));
        totalOttLabel.setBounds (countCell.withSizeKeepingCentre (74, 13).translated (0, 13));
        preRow.removeFromLeft (midGapX + 12);

        xLow.setBounds  (preRow.removeFromLeft (midKnobW)); preRow.removeFromLeft (midGapX);
        xHigh.setBounds (preRow.removeFromLeft (midKnobW));

        area.removeFromTop (6);

        masterLabel.setBounds (area.removeFromTop (16));
        area.removeFromTop (2);

        auto mRow = area.removeFromTop (60);
        postHPF.setBounds   (mRow.removeFromLeft (midKnobW)); mRow.removeFromLeft (midGapX);
        postLPF.setBounds   (mRow.removeFromLeft (midKnobW)); mRow.removeFromLeft (midGapX + 10);

        phaseModeBox.setBounds (mRow.removeFromLeft (130).withSizeKeepingCentre (125, 26));
        mRow.removeFromLeft (midGapX + 12);

        dryWet.setBounds    (mRow.removeFromLeft (midKnobW)); mRow.removeFromLeft (midGapX);
        outGain.setBounds   (mRow.removeFromLeft (midKnobW)); mRow.removeFromLeft (midGapX);
        limitCeil.setBounds (mRow.removeFromLeft (midKnobW));

        area.removeFromTop (10);

        // 3. 下段 STAGE CONTROLS (上下対 ＆ グループ機能レイアウト)
        auto stageHeader = area.removeFromTop (22);
        stageLabel.setBounds (stageHeader.removeFromLeft (180));
        if (activeStage == 1)
            s1OnBtn.setBounds (stageHeader.removeFromLeft (90).withSizeKeepingCentre (85, 24));
        else
            s2OnBtn.setBounds (stageHeader.removeFromLeft (90).withSizeKeepingCentre (85, 24));

        area.removeFromTop (4);

        ArcKnob* gn = (activeStage == 1) ? s1Gain : s2Gain;
        ArcKnob* up = (activeStage == 1) ? s1Up   : s2Up;
        ArcKnob* dn = (activeStage == 1) ? s1Dn   : s2Dn;
        ArcKnob& tm = (activeStage == 1) ? s1Time : s2Time;
        ArcKnob& mx = (activeStage == 1) ? s1Mix  : s2Mix;
        ArcKnob* ak = (activeStage == 1) ? s1Atk  : s2Atk;
        ArcKnob* rl = (activeStage == 1) ? s1Rel  : s2Rel;

        // ノブサイズ 80px
        int kW = 80;
        int kG = 12;

        // --- 行1: GAIN L/M/H | UPWARD L/M/H | ATTACK L/M/H | TIME (全8ノブ) ---
        auto sRow1 = area.removeFromTop (72);

        // GAIN (LOW, MID, HI)
        gn[0].setBounds (sRow1.removeFromLeft (kW)); sRow1.removeFromLeft (kG);
        gn[1].setBounds (sRow1.removeFromLeft (kW)); sRow1.removeFromLeft (kG);
        gn[2].setBounds (sRow1.removeFromLeft (kW)); sRow1.removeFromLeft (kG + 14);

        // UPWARD (LOW UP, MID UP, HI UP)  ← Downwardと上下対！
        up[0].setBounds (sRow1.removeFromLeft (kW)); sRow1.removeFromLeft (kG);
        up[1].setBounds (sRow1.removeFromLeft (kW)); sRow1.removeFromLeft (kG);
        up[2].setBounds (sRow1.removeFromLeft (kW)); sRow1.removeFromLeft (kG + 14);

        // ATTACK (ATK L, ATK M, ATK H)    ← Releaseと上下対！
        ak[0].setBounds (sRow1.removeFromLeft (kW)); sRow1.removeFromLeft (kG);
        ak[1].setBounds (sRow1.removeFromLeft (kW)); sRow1.removeFromLeft (kG);
        ak[2].setBounds (sRow1.removeFromLeft (kW)); sRow1.removeFromLeft (kG + 14);

        // TIME
        tm.setBounds    (sRow1.removeFromLeft (kW));

        area.removeFromTop (6);

        // --- 行2: (空き) | DOWNWARD L/M/H | RELEASE L/M/H | MIX (全8ノブ) ---
        auto sRow2 = area.removeFromTop (72);

        // GAINの下はスペース空け (またはMIX移動)
        sRow2.removeFromLeft (kW * 3 + kG * 2 + 14);

        // DOWNWARD (LOW DN, MID DN, HI DN) ← UPWARDの真下に整列！
        dn[0].setBounds (sRow2.removeFromLeft (kW)); sRow2.removeFromLeft (kG);
        dn[1].setBounds (sRow2.removeFromLeft (kW)); sRow2.removeFromLeft (kG);
        dn[2].setBounds (sRow2.removeFromLeft (kW)); sRow2.removeFromLeft (kG + 14);

        // RELEASE (REL L, REL M, REL H)    ← ATTACKの真下に整列！(REL Hも完全表示)
        rl[0].setBounds (sRow2.removeFromLeft (kW)); sRow2.removeFromLeft (kG);
        rl[1].setBounds (sRow2.removeFromLeft (kW)); sRow2.removeFromLeft (kG);
        rl[2].setBounds (sRow2.removeFromLeft (kW)); sRow2.removeFromLeft (kG + 14);

        // MIX
        mx.setBounds    (sRow2.removeFromLeft (kW));
    }

private:
    int activeStage = 1;

    juce::TextButton preDriveBtn { "PRE-DRIVE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> preDriveAt;

    juce::ComboBox totalOttBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> totalOttAttachment;
    juce::Label totalOttLabel;

    ArcKnob inGain, drive, oddBlend, evenBlend;
    ArcKnob xLow, xHigh;

    ArcKnob postHPF, postLPF, dryWet, outGain, limitCeil;
    juce::ComboBox phaseModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> phaseModeAttachment;

    DynVisualComponent dynS1, dynS2;

    juce::TextButton s1OnBtn, s2OnBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> s1OnAt, s2OnAt;

    ArcKnob s1Gain[3], s1Up[3], s1Dn[3], s1Time, s1Mix, s1Atk[3], s1Rel[3];
    ArcKnob s2Gain[3], s2Up[3], s2Dn[3], s2Time, s2Mix, s2Atk[3], s2Rel[3];

    juce::Label preLabel, masterLabel, stageLabel;

    void updateStageVisibility()
    {
        bool isS1 = (activeStage == 1);
        s1OnBtn.setVisible (isS1);
        s2OnBtn.setVisible (!isS1);

        for (int i = 0; i < 3; ++i)
        {
            s1Gain[i].setVisible (isS1); s1Up[i].setVisible (isS1); s1Dn[i].setVisible (isS1);
            s1Atk[i].setVisible (isS1);  s1Rel[i].setVisible (isS1);

            s2Gain[i].setVisible (!isS1); s2Up[i].setVisible (!isS1); s2Dn[i].setVisible (!isS1);
            s2Atk[i].setVisible (!isS1);  s2Rel[i].setVisible (!isS1);
        }
        s1Time.setVisible (isS1); s1Mix.setVisible (isS1);
        s2Time.setVisible (!isS1); s2Mix.setVisible (!isS1);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};
