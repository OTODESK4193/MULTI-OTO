#include "PluginProcessor.h"
#include "PluginEditor.h"

MultiOtoAudioProcessorEditor::MultiOtoAudioProcessorEditor(MultiOtoAudioProcessor& p) : AudioProcessorEditor(&p), audioProcessor(p) {
    setOpaque(true);
    auto& apvts = audioProcessor.apvts;

    auto setupBtn = [&](juce::TextButton& b, const char* pID, std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& a) {
        b.setClickingTogglesState(true);
        b.setColour(juce::TextButton::buttonColourId, MultiOtoColors::Surface);
        b.setColour(juce::TextButton::buttonOnColourId, MultiOtoColors::Accent);
        b.setColour(juce::TextButton::textColourOffId, MultiOtoColors::TextSecondary);
        b.setColour(juce::TextButton::textColourOnId, MultiOtoColors::Background);
        addAndMakeVisible(b);
        a = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, pID, b);
        };

    setupBtn(preDriveBtn, "predrive_on", preDriveAt);
    setupBtn(stage1Btn, "s1_on", s1At);
    setupBtn(stage2Btn, "s2_on", s2At);

    totalOttBox.addItemList({ "2","4","8","16","32","64" }, 1);
    totalOttBox.setLookAndFeel(&laf);
    addAndMakeVisible(totalOttBox);
    totalOttAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "total_ott", totalOttBox);

    totalOttLabel.setText("COUNT", juce::dontSendNotification);
    totalOttLabel.setColour(juce::Label::textColourId, MultiOtoColors::TextSecondary);
    totalOttLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(totalOttLabel);

    // ノブのビルド（配列ポインタを廃止し、すべて愚直に一つずつ生成）
    inGain.build(apvts, "in_gain", "IN", this, laf);
    drive.build(apvts, "drive", "DRIVE", this, laf);
    oddBlend.build(apvts, "odd_blend", "ODD", this, laf);
    evenBlend.build(apvts, "even_blend", "EVEN", this, laf);
    xLow.build(apvts, "xover_low", "LOW X", this, laf);
    xHigh.build(apvts, "xover_high", "HIGH X", this, laf);

    s1GainL.build(apvts, "s1_gain_l", "LOW G", this, laf);
    s1GainM.build(apvts, "s1_gain_m", "MID G", this, laf);
    s1GainH.build(apvts, "s1_gain_h", "HI G", this, laf);
    s1DepthL.build(apvts, "s1_depth_l", "LOW D", this, laf);
    s1DepthM.build(apvts, "s1_depth_m", "MID D", this, laf);
    s1DepthH.build(apvts, "s1_depth_h", "HI D", this, laf);
    s1Time.build(apvts, "s1_time", "TIME", this, laf);
    s1Mix.build(apvts, "s1_mix", "MIX", this, laf);
    s1AtkL.build(apvts, "s1_atk_l", "ATK L", this, laf);
    s1AtkM.build(apvts, "s1_atk_m", "ATK M", this, laf);
    s1AtkH.build(apvts, "s1_atk_h", "ATK H", this, laf);
    s1RelL.build(apvts, "s1_rel_l", "REL L", this, laf);
    s1RelM.build(apvts, "s1_rel_m", "REL M", this, laf);
    s1RelH.build(apvts, "s1_rel_h", "REL H", this, laf);

    s2GainL.build(apvts, "s2_gain_l", "LOW G", this, laf);
    s2GainM.build(apvts, "s2_gain_m", "MID G", this, laf);
    s2GainH.build(apvts, "s2_gain_h", "HI G", this, laf);
    s2DepthL.build(apvts, "s2_depth_l", "LOW D", this, laf);
    s2DepthM.build(apvts, "s2_depth_m", "MID D", this, laf);
    s2DepthH.build(apvts, "s2_depth_h", "HI D", this, laf);
    s2Time.build(apvts, "s2_time", "TIME", this, laf);
    s2Mix.build(apvts, "s2_mix", "MIX", this, laf);
    s2AtkL.build(apvts, "s2_atk_l", "ATK L", this, laf);
    s2AtkM.build(apvts, "s2_atk_m", "ATK M", this, laf);
    s2AtkH.build(apvts, "s2_atk_h", "ATK H", this, laf);
    s2RelL.build(apvts, "s2_rel_l", "REL L", this, laf);
    s2RelM.build(apvts, "s2_rel_m", "REL M", this, laf);
    s2RelH.build(apvts, "s2_rel_h", "REL H", this, laf);

    postHPF.build(apvts, "post_hpf", "HPF", this, laf);
    postLPF.build(apvts, "post_lpf", "LPF", this, laf);
    dryWet.build(apvts, "dry_wet", "MIX", this, laf);
    outGain.build(apvts, "out_gain", "OUT", this, laf);
    limitCeil.build(apvts, "limit_ceil", "CEIL", this, laf);

    phaseModeBox.addItemList({ "COLOR PHASE", "ALIGN PHASE" }, 1);
    phaseModeBox.setLookAndFeel(&laf);
    phaseModeBox.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(phaseModeBox);
    phaseModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "phase_mode", phaseModeBox);

    addAndMakeVisible(preDriveGroup); preDriveGroup.setText("PRE-DRIVE & X-OVER");
    addAndMakeVisible(stage1Group); stage1Group.setText("STAGE 1");
    addAndMakeVisible(stage2Group); stage2Group.setText("STAGE 2");
    addAndMakeVisible(masterGroup); masterGroup.setText("MASTER");

    s1AdvBtn.addListener(this); addAndMakeVisible(s1AdvBtn);
    s2AdvBtn.addListener(this); addAndMakeVisible(s2AdvBtn);
    s1AdvBtn.setColour(juce::TextButton::buttonColourId, MultiOtoColors::Surface);
    s2AdvBtn.setColour(juce::TextButton::buttonColourId, MultiOtoColors::Surface);

    // ピクセルパーフェクトな横幅 895 (余白なし) に設定
    setSize(895, 750);
}

MultiOtoAudioProcessorEditor::~MultiOtoAudioProcessorEditor() {
    totalOttBox.setLookAndFeel(nullptr);
    phaseModeBox.setLookAndFeel(nullptr);
    removeAllChildren();
}

void MultiOtoAudioProcessorEditor::buttonClicked(juce::Button* b) {
    if (b == &s1AdvBtn) s1AdvOpen = !s1AdvOpen;
    if (b == &s2AdvBtn) s2AdvOpen = !s2AdvOpen;
    resized();
}

void MultiOtoAudioProcessorEditor::paint(juce::Graphics& g) {
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xFF170801), 0, 0, juce::Colour(0xFF1c0901), 0, (float)getHeight(), false));
    g.fillAll();
}

void MultiOtoAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced(20); // 左右上下に 20px のマージン
    int kS = 90;  // 全ノブを 90x90px に完全統一
    int gap = 15; // ノブ間の隙間

    // ==========================================
    // 上段エリア (PreDrive + Master)
    // ==========================================
    auto topArea = area.removeFromTop(230); // 2段組に十分な高さを確保

    // PreDrive幅: ノブ4つ(360) + 隙間3つ(45) + グループ内余白左右(30) = 435px
    preDriveGroup.setBounds(topArea.removeFromLeft(435));

    auto pA = preDriveGroup.getBounds().reduced(15).withTrimmedTop(15);

    // PreDrive 1段目: In, Drive, ODD, Even
    auto pR1 = pA.removeFromTop(kS);
    inGain.setBounds(pR1.removeFromLeft(kS)); pR1.removeFromLeft(gap);
    drive.setBounds(pR1.removeFromLeft(kS)); pR1.removeFromLeft(gap);
    oddBlend.setBounds(pR1.removeFromLeft(kS)); pR1.removeFromLeft(gap);
    evenBlend.setBounds(pR1.removeFromLeft(kS));

    // PreDrive 2段目: ON, Count, LowX, HighX
    auto pR2 = pA.removeFromTop(kS);
    auto onCell = pR2.removeFromLeft(kS);
    preDriveBtn.setBounds(onCell.withSizeKeepingCentre(60, 24));
    pR2.removeFromLeft(gap);

    auto countCell = pR2.removeFromLeft(kS);
    totalOttBox.setBounds(countCell.withSizeKeepingCentre(75, 26).translated(0, -5));
    totalOttLabel.setBounds(countCell.withSizeKeepingCentre(75, 15).translated(0, 15));
    pR2.removeFromLeft(gap);

    xLow.setBounds(pR2.removeFromLeft(kS)); pR2.removeFromLeft(gap);
    xHigh.setBounds(pR2.removeFromLeft(kS));

    // PreDriveとMasterの間の空間 (右端をStageの右端とピッタリ合わせるため 60px)
    topArea.removeFromLeft(60);

    // Master幅: HPF, LPF, Phaseの3要素。ノブ2つ(180)+Phase(120)+隙間(30)+余白(30) = 360px
    masterGroup.setBounds(topArea.removeFromLeft(360));
    auto mA = masterGroup.getBounds().reduced(15).withTrimmedTop(15);

    auto mR1 = mA.removeFromTop(kS);
    postHPF.setBounds(mR1.removeFromLeft(kS)); mR1.removeFromLeft(gap);
    postLPF.setBounds(mR1.removeFromLeft(kS)); mR1.removeFromLeft(gap);
    phaseModeBox.setBounds(mR1.removeFromLeft(120).withSizeKeepingCentre(115, 26).translated(0, -5));

    auto mR2 = mA.removeFromTop(kS);
    dryWet.setBounds(mR2.removeFromLeft(kS)); mR2.removeFromLeft(gap);
    limitCeil.setBounds(mR2.removeFromLeft(kS)); mR2.removeFromLeft(gap);
    outGain.setBounds(mR2.removeFromLeft(kS));

    area.removeFromTop(10);

    // ==========================================
    // Stage レイアウト関数 (安全な参照渡し)
    // ==========================================
    auto layoutS = [&](juce::GroupComponent& g, juce::TextButton& onBtn, juce::TextButton& advBtn, bool open,
        ArcKnob& gL, ArcKnob& gM, ArcKnob& gH, ArcKnob& time,
        ArcKnob& dL, ArcKnob& dM, ArcKnob& dH, ArcKnob& mix,
        ArcKnob& aL, ArcKnob& aM, ArcKnob& aH,
        ArcKnob& rL, ArcKnob& rM, ArcKnob& rH,
        juce::Rectangle<int> b) {
            g.setBounds(b);
            auto sA = b.reduced(15).withTrimmedTop(15);

            // 左カラム: ON(上), ADV(下)
            auto btnCol = sA.removeFromLeft(kS);
            onBtn.setBounds(btnCol.removeFromTop(kS).withSizeKeepingCentre(60, 24));
            advBtn.setBounds(btnCol.removeFromTop(kS).withSizeKeepingCentre(80, 24));

            sA.removeFromLeft(gap);

            // Basicカラム: Gain/Time, Depth/Mix
            auto basicCol = sA.removeFromLeft(kS * 4 + gap * 3);

            auto bR1 = basicCol.removeFromTop(kS);
            gL.setBounds(bR1.removeFromLeft(kS)); bR1.removeFromLeft(gap);
            gM.setBounds(bR1.removeFromLeft(kS)); bR1.removeFromLeft(gap);
            gH.setBounds(bR1.removeFromLeft(kS)); bR1.removeFromLeft(gap);
            time.setBounds(bR1.removeFromLeft(kS));

            auto bR2 = basicCol.removeFromTop(kS);
            dL.setBounds(bR2.removeFromLeft(kS)); bR2.removeFromLeft(gap);
            dM.setBounds(bR2.removeFromLeft(kS)); bR2.removeFromLeft(gap);
            dH.setBounds(bR2.removeFromLeft(kS)); bR2.removeFromLeft(gap);
            mix.setBounds(bR2.removeFromLeft(kS));

            sA.removeFromLeft(gap);

            aL.setVisible(open); aM.setVisible(open); aH.setVisible(open);
            rL.setVisible(open); rM.setVisible(open); rH.setVisible(open);

            if (open) {
                // Advancedカラム: Attack/Release
                auto advCol = sA.removeFromLeft(kS * 3 + gap * 2);

                auto aR1 = advCol.removeFromTop(kS);
                aL.setBounds(aR1.removeFromLeft(kS)); aR1.removeFromLeft(gap);
                aM.setBounds(aR1.removeFromLeft(kS)); aR1.removeFromLeft(gap);
                aH.setBounds(aR1.removeFromLeft(kS));

                auto aR2 = advCol.removeFromTop(kS);
                rL.setBounds(aR2.removeFromLeft(kS)); aR2.removeFromLeft(gap);
                rM.setBounds(aR2.removeFromLeft(kS)); aR2.removeFromLeft(gap);
                rH.setBounds(aR2.removeFromLeft(kS));
            }
        };

    layoutS(stage1Group, stage1Btn, s1AdvBtn, s1AdvOpen,
        s1GainL, s1GainM, s1GainH, s1Time,
        s1DepthL, s1DepthM, s1DepthH, s1Mix,
        s1AtkL, s1AtkM, s1AtkH,
        s1RelL, s1RelM, s1RelH,
        area.removeFromTop(230));

    area.removeFromTop(10);

    layoutS(stage2Group, stage2Btn, s2AdvBtn, s2AdvOpen,
        s2GainL, s2GainM, s2GainH, s2Time,
        s2DepthL, s2DepthM, s2DepthH, s2Mix,
        s2AtkL, s2AtkM, s2AtkH,
        s2RelL, s2RelM, s2RelH,
        area.removeFromTop(230));
}