#include "PluginProcessor.h"
#include "PluginEditor.h"

MultiOtoAudioProcessorEditor::MultiOtoAudioProcessorEditor(MultiOtoAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
    auto& apvts = audioProcessor.apvts;

    // --- Build Components ---
    totalOtt.build(apvts, "total_ott", "COUNT", this, laf);
    inGain.build(apvts, "in_gain", "IN", this, laf);
    drive.build(apvts, "drive", "DRIVE", this, laf);
    oddBlend.build(apvts, "odd_blend", "ODD", this, laf);
    evenBlend.build(apvts, "even_blend", "EVEN", this, laf);

    xLow.build(apvts, "xover_low", "LOW X", this, laf);
    xHigh.build(apvts, "xover_high", "HIGH X", this, laf);

    s1GainH.build(apvts, "s1_gain_h", "HI G", this, laf);
    s1GainM.build(apvts, "s1_gain_m", "MID G", this, laf);
    s1GainL.build(apvts, "s1_gain_l", "LOW G", this, laf);
    s1DepthH.build(apvts, "s1_depth_h", "HI D", this, laf);
    s1DepthM.build(apvts, "s1_depth_m", "MID D", this, laf);
    s1DepthL.build(apvts, "s1_depth_l", "LOW D", this, laf);
    s1Time.build(apvts, "s1_time", "TIME", this, laf);
    s1AtkH.build(apvts, "s1_atk_h", "ATK H", this, laf); s1AtkM.build(apvts, "s1_atk_m", "ATK M", this, laf); s1AtkL.build(apvts, "s1_atk_l", "ATK L", this, laf);
    s1RelH.build(apvts, "s1_rel_h", "REL H", this, laf); s1RelM.build(apvts, "s1_rel_m", "REL M", this, laf); s1RelL.build(apvts, "s1_rel_l", "REL L", this, laf);

    s2GainH.build(apvts, "s2_gain_h", "HI G", this, laf);
    s2GainM.build(apvts, "s2_gain_m", "MID G", this, laf);
    s2GainL.build(apvts, "s2_gain_l", "LOW G", this, laf);
    s2DepthH.build(apvts, "s2_depth_h", "HI D", this, laf);
    s2DepthM.build(apvts, "s2_depth_m", "MID D", this, laf);
    s2DepthL.build(apvts, "s2_depth_l", "LOW D", this, laf);
    s2Time.build(apvts, "s2_time", "TIME", this, laf);
    s2AtkH.build(apvts, "s2_atk_h", "ATK H", this, laf); s2AtkM.build(apvts, "s2_atk_m", "ATK M", this, laf); s2AtkL.build(apvts, "s2_atk_l", "ATK L", this, laf);
    s2RelH.build(apvts, "s2_rel_h", "REL H", this, laf); s2RelM.build(apvts, "s2_rel_m", "REL M", this, laf); s2RelL.build(apvts, "s2_rel_l", "REL L", this, laf);

    postHPF.build(apvts, "post_hpf", "HPF", this, laf);
    postLPF.build(apvts, "post_lpf", "LPF", this, laf);
    dryWet.build(apvts, "dry_wet", "MIX", this, laf);
    outGain.build(apvts, "out_gain", "OUT", this, laf);
    limitCeil.build(apvts, "limit_ceil", "CEIL", this, laf);

    addAndMakeVisible(preDriveGroup); preDriveGroup.setText("PRE-DRIVE");
    addAndMakeVisible(stage1Group); stage1Group.setText("STAGE 1");
    addAndMakeVisible(stage2Group); stage2Group.setText("STAGE 2");
    addAndMakeVisible(masterGroup); masterGroup.setText("MASTER");

    s1AdvBtn.addListener(this); addAndMakeVisible(s1AdvBtn);
    s2AdvBtn.addListener(this); addAndMakeVisible(s2AdvBtn);

    // Advancedを展開しても余裕のあるサイズに拡張
    setSize(1080, 500);
}

MultiOtoAudioProcessorEditor::~MultiOtoAudioProcessorEditor() = default;

void MultiOtoAudioProcessorEditor::buttonClicked(juce::Button* b) {
    if (b == &s1AdvBtn) s1AdvOpen = !s1AdvOpen;
    if (b == &s2AdvBtn) s2AdvOpen = !s2AdvOpen;
    resized(); // ボタン押下でレイアウトを再計算
}

void MultiOtoAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(MultiOtoColors::Background);
}

void MultiOtoAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced(15);
    int kw = 70; // ノブの基本サイズ（幅・高さ）

    // ==========================================================
    // ROW 1: HEADER (Pre-Drive & Crossover)
    // ==========================================================
    auto headerRow = area.removeFromTop(85);

    // Pre-Drive Group
    preDriveGroup.setBounds(headerRow.removeFromLeft(420));
    // getBounds() を使って絶対座標を取得
    auto preArea = preDriveGroup.getBounds().reduced(10).withTrimmedTop(20);

    inGain.setBounds(preArea.removeFromLeft(kw));   preArea.removeFromLeft(5);
    drive.setBounds(preArea.removeFromLeft(kw));    preArea.removeFromLeft(5);
    oddBlend.setBounds(preArea.removeFromLeft(kw)); preArea.removeFromLeft(5);
    evenBlend.setBounds(preArea.removeFromLeft(kw)); preArea.removeFromLeft(5);
    totalOtt.setBounds(preArea.removeFromLeft(kw));

    headerRow.removeFromLeft(20); // セクション間のスペーサー

    // Crossover (グループ枠なしで配置)
    xLow.setBounds(headerRow.removeFromLeft(kw));   headerRow.removeFromLeft(10);
    xHigh.setBounds(headerRow.removeFromLeft(kw));

    area.removeFromTop(15); // ヘッダー下のスペーサー

    // ==========================================================
    // ROW 2 & 3: MAIN (Master on Right, Stages on Left)
    // ==========================================================
    auto rightArea = area.removeFromRight(300); // マスターセクション用に右側を確保
    area.removeFromRight(20); // ステージとマスター間のスペーサー

    // --- Master Section ---
    masterGroup.setBounds(rightArea);
    auto mArea = masterGroup.getBounds().reduced(15).withTrimmedTop(25);
    auto mRow1 = mArea.removeFromTop(75);
    mArea.removeFromTop(10);
    auto mRow2 = mArea.removeFromTop(75);

    postHPF.setBounds(mRow1.removeFromLeft(kw)); mRow1.removeFromLeft(15);
    postLPF.setBounds(mRow1.removeFromLeft(kw));

    dryWet.setBounds(mRow2.removeFromLeft(kw));    mRow2.removeFromLeft(15);
    limitCeil.setBounds(mRow2.removeFromLeft(kw)); mRow2.removeFromLeft(15);
    outGain.setBounds(mRow2.removeFromLeft(kw));

    // --- Stages Section ---
    // ステージをレイアウトするためのラムダ関数
    auto layoutStage = [&](juce::GroupComponent& group, bool open,
        ArcKnob& gH, ArcKnob& gM, ArcKnob& gL,
        ArcKnob& dH, ArcKnob& dM, ArcKnob& dL, ArcKnob& time,
        ArcKnob& aH, ArcKnob& aM, ArcKnob& aL,
        ArcKnob& rH, ArcKnob& rM, ArcKnob& rL, juce::TextButton& btn,
        juce::Rectangle<int> bounds)
        {
            group.setBounds(bounds);
            // ボタンをグループ枠の右上に配置
            btn.setBounds(bounds.getRight() - 90, bounds.getY() + 2, 80, 18);

            auto sArea = bounds.reduced(15).withTrimmedTop(20);

            // Basic (Gain & Depth)
            auto basicArea = sArea.removeFromLeft(230);
            auto bRow1 = basicArea.removeFromTop(70);
            basicArea.removeFromTop(5);
            auto bRow2 = basicArea.removeFromTop(70);

            gL.setBounds(bRow1.removeFromLeft(kw)); bRow1.removeFromLeft(10);
            gM.setBounds(bRow1.removeFromLeft(kw)); bRow1.removeFromLeft(10);
            gH.setBounds(bRow1.removeFromLeft(kw));

            dL.setBounds(bRow2.removeFromLeft(kw)); bRow2.removeFromLeft(10);
            dM.setBounds(bRow2.removeFromLeft(kw)); bRow2.removeFromLeft(10);
            dH.setBounds(bRow2.removeFromLeft(kw));

            sArea.removeFromLeft(15);

            // Macro Time (中央に配置)
            auto timeArea = sArea.removeFromLeft(kw);
            time.setBounds(timeArea.withSizeKeepingCentre(kw, kw));

            sArea.removeFromLeft(30); // Advancedセクションへのスペーサー

            // Advancedの表示制御
            aL.setVisible(open); aM.setVisible(open); aH.setVisible(open);
            rL.setVisible(open); rM.setVisible(open); rH.setVisible(open);

            // Advanced (Attack & Release)
            if (open) {
                auto advArea = sArea.removeFromLeft(230);
                auto aRow1 = advArea.removeFromTop(70);
                advArea.removeFromTop(5);
                auto aRow2 = advArea.removeFromTop(70);

                aL.setBounds(aRow1.removeFromLeft(kw)); aRow1.removeFromLeft(10);
                aM.setBounds(aRow1.removeFromLeft(kw)); aRow1.removeFromLeft(10);
                aH.setBounds(aRow1.removeFromLeft(kw));

                rL.setBounds(aRow2.removeFromLeft(kw)); aRow2.removeFromLeft(10);
                rM.setBounds(aRow2.removeFromLeft(kw)); aRow2.removeFromLeft(10);
                rH.setBounds(aRow2.removeFromLeft(kw));
            }
        };

    auto stage1Bounds = area.removeFromTop(180);
    area.removeFromTop(10);
    auto stage2Bounds = area.removeFromTop(180);

    layoutStage(stage1Group, s1AdvOpen, s1GainH, s1GainM, s1GainL, s1DepthH, s1DepthM, s1DepthL, s1Time, s1AtkH, s1AtkM, s1AtkL, s1RelH, s1RelM, s1RelL, s1AdvBtn, stage1Bounds);
    layoutStage(stage2Group, s2AdvOpen, s2GainH, s2GainM, s2GainL, s2DepthH, s2DepthM, s2DepthL, s2Time, s2AtkH, s2AtkM, s2AtkL, s2RelH, s2RelM, s2RelL, s2AdvBtn, stage2Bounds);
}