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

    inGain.build(apvts, "in_gain", "IN", this, laf);
    drive.build(apvts, "drive", "DRIVE", this, laf);
    oddBlend.build(apvts, "odd_blend", "ODD", this, laf);
    evenBlend.build(apvts, "even_blend", "EVEN", this, laf);
    xLow.build(apvts, "xover_low", "LOW X", this, laf);
    xHigh.build(apvts, "xover_high", "HIGH X", this, laf);

    auto buildS = [&](int s) {
        juce::String st = juce::String(s);
        ArcKnob* gn = (s == 1) ? &s1GainL : &s2GainL;
        ArcKnob* dp = (s == 1) ? &s1DepthL : &s2DepthL;
        ArcKnob* ak = (s == 1) ? &s1AtkL : &s2AtkL;
        ArcKnob* rl = (s == 1) ? &s1RelL : &s2RelL;

        gn[0].build(apvts, "s" + st + "_gain_l", "LOW G", this, laf);
        gn[1].build(apvts, "s" + st + "_gain_m", "MID G", this, laf);
        gn[2].build(apvts, "s" + st + "_gain_h", "HI G", this, laf);
        dp[0].build(apvts, "s" + st + "_depth_l", "LOW D", this, laf);
        dp[1].build(apvts, "s" + st + "_depth_m", "MID D", this, laf);
        dp[2].build(apvts, "s" + st + "_depth_h", "HI D", this, laf);
        ak[0].build(apvts, "s" + st + "_atk_l", "ATK L", this, laf);
        ak[1].build(apvts, "s" + st + "_atk_m", "ATK M", this, laf);
        ak[2].build(apvts, "s" + st + "_atk_h", "ATK H", this, laf);
        rl[0].build(apvts, "s" + st + "_rel_l", "REL L", this, laf);
        rl[1].build(apvts, "s" + st + "_rel_m", "REL M", this, laf);
        rl[2].build(apvts, "s" + st + "_rel_h", "REL H", this, laf);

        if (s == 1) {
            s1Time.build(apvts, "s1_time", "TIME", this, laf);
            s1Mix.build(apvts, "s1_mix", "MIX", this, laf);
        }
        else {
            s2Time.build(apvts, "s2_time", "TIME", this, laf);
            s2Mix.build(apvts, "s2_mix", "MIX", this, laf);
        }
        };

    buildS(1); buildS(2);

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

    setSize(865, 710); // 余白を完全に計算し直したジャストサイズ
}

MultiOtoAudioProcessorEditor::~MultiOtoAudioProcessorEditor() {
    totalOttBox.setLookAndFeel(nullptr);
    phaseModeBox.setLookAndFeel(nullptr);
    removeAllChildren();
}

void MultiOtoAudioProcessorEditor::paint(juce::Graphics& g) {
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xFF170801), 0, 0, juce::Colour(0xFF1c0901), 0, (float)getHeight(), false));
    g.fillAll();
}

void MultiOtoAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced(20);
    int kS = 90; // すべてのノブを90pxに完全統一
    int gap = 15; // 隙間

    auto topArea = area.removeFromTop(210);

    preDriveGroup.setBounds(topArea.removeFromLeft(kS * 4 + gap * 3 + 30));
    auto pA = preDriveGroup.getBounds().reduced(15).withTrimmedTop(15);

    auto pR1 = pA.removeFromTop(kS);
    inGain.setBounds(pR1.removeFromLeft(kS)); pR1.removeFromLeft(gap);
    drive.setBounds(pR1.removeFromLeft(kS)); pR1.removeFromLeft(gap);
    oddBlend.setBounds(pR1.removeFromLeft(kS)); pR1.removeFromLeft(gap);
    evenBlend.setBounds(pR1.removeFromLeft(kS));

    auto pR2 = pA.removeFromTop(kS);
    preDriveBtn.setBounds(pR2.removeFromLeft(kS).withSizeKeepingCentre(60, 24));
    pR2.removeFromLeft(gap);

    auto countArea = pR2.removeFromLeft(kS);
    totalOttBox.setBounds(countArea.withSizeKeepingCentre(75, 28).translated(0, -5));
    totalOttLabel.setBounds(countArea.withSizeKeepingCentre(75, 15).translated(0, 15));
    pR2.removeFromLeft(gap);

    xLow.setBounds(pR2.removeFromLeft(kS)); pR2.removeFromLeft(gap);
    xHigh.setBounds(pR2.removeFromLeft(kS));

    topArea.removeFromLeft(40);

    masterGroup.setBounds(topArea.removeFromLeft(330));
    auto mA = masterGroup.getBounds().reduced(15).withTrimmedTop(15);

    auto mR1 = mA.removeFromTop(kS);
    postHPF.setBounds(mR1.removeFromLeft(kS)); mR1.removeFromLeft(gap);
    postLPF.setBounds(mR1.removeFromLeft(kS)); mR1.removeFromLeft(gap);
    phaseModeBox.setBounds(mR1.removeFromLeft(120).withSizeKeepingCentre(110, 28).translated(-5, -5));

    auto mR2 = mA.removeFromTop(kS);
    dryWet.setBounds(mR2.removeFromLeft(kS)); mR2.removeFromLeft(gap);
    limitCeil.setBounds(mR2.removeFromLeft(kS)); mR2.removeFromLeft(gap);
    outGain.setBounds(mR2.removeFromLeft(kS));

    area.removeFromTop(10);

    auto layoutS = [&](juce::GroupComponent& g, juce::TextButton& onBtn,
        ArcKnob& gL, ArcKnob& gM, ArcKnob& gH, ArcKnob& time,
        ArcKnob& dL, ArcKnob& dM, ArcKnob& dH, ArcKnob& mix,
        ArcKnob& aL, ArcKnob& aM, ArcKnob& aH,
        ArcKnob& rL, ArcKnob& rM, ArcKnob& rH,
        juce::Rectangle<int> b) {
            g.setBounds(b);
            auto sA = b.reduced(15).withTrimmedTop(15);

            auto btnCol = sA.removeFromLeft(kS);
            onBtn.setBounds(btnCol.withSizeKeepingCentre(60, 24));

            sA.removeFromLeft(gap);

            // 全7カラムのノブを平坦に並べる (7 * 90) + (6 * 15) = 630px + 90px = 720px
            auto bA = sA.removeFromLeft(kS * 7 + gap * 6);

            auto bR1 = bA.removeFromTop(kS);
            gL.setBounds(bR1.removeFromLeft(kS)); bR1.removeFromLeft(gap);
            gM.setBounds(bR1.removeFromLeft(kS)); bR1.removeFromLeft(gap);
            gH.setBounds(bR1.removeFromLeft(kS)); bR1.removeFromLeft(gap);
            time.setBounds(bR1.removeFromLeft(kS)); bR1.removeFromLeft(gap);
            aL.setBounds(bR1.removeFromLeft(kS)); bR1.removeFromLeft(gap);
            aM.setBounds(bR1.removeFromLeft(kS)); bR1.removeFromLeft(gap);
            aH.setBounds(bR1.removeFromLeft(kS));

            auto bR2 = bA.removeFromTop(kS);
            dL.setBounds(bR2.removeFromLeft(kS)); bR2.removeFromLeft(gap);
            dM.setBounds(bR2.removeFromLeft(kS)); bR2.removeFromLeft(gap);
            dH.setBounds(bR2.removeFromLeft(kS)); bR2.removeFromLeft(gap);
            mix.setBounds(bR2.removeFromLeft(kS)); bR2.removeFromLeft(gap);
            rL.setBounds(bR2.removeFromLeft(kS)); bR2.removeFromLeft(gap);
            rM.setBounds(bR2.removeFromLeft(kS)); bR2.removeFromLeft(gap);
            rH.setBounds(bR2.removeFromLeft(kS));
        };

    layoutS(stage1Group, stage1Btn,
        s1GainL, s1GainM, s1GainH, s1Time,
        s1DepthL, s1DepthM, s1DepthH, s1Mix,
        s1AtkL, s1AtkM, s1AtkH,
        s1RelL, s1RelM, s1RelH,
        area.removeFromTop(210));

    area.removeFromTop(10);

    layoutS(stage2Group, stage2Btn,
        s2GainL, s2GainM, s2GainH, s2Time,
        s2DepthL, s2DepthM, s2DepthH, s2Mix,
        s2AtkL, s2AtkM, s2AtkH,
        s2RelL, s2RelM, s2RelH,
        area.removeFromTop(210));
}