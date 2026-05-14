#include "PluginProcessor.h"
#include "PluginEditor.h"

MultiOtoAudioProcessorEditor::MultiOtoAudioProcessorEditor(MultiOtoAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {

    // Direct2D パフォーマンス最適化
    setOpaque(true);

    auto& apvts = audioProcessor.apvts;

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

    phaseModeBox.addItemList({ "COLOR PHASE", "ALIGN PHASE" }, 1);
    phaseModeBox.setJustificationType(juce::Justification::centred);
    phaseModeBox.setLookAndFeel(&laf);
    addAndMakeVisible(phaseModeBox);
    phaseModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "phase_mode", phaseModeBox);

    addAndMakeVisible(preDriveGroup); preDriveGroup.setText("PRE-DRIVE");
    addAndMakeVisible(stage1Group); stage1Group.setText("STAGE 1");
    addAndMakeVisible(stage2Group); stage2Group.setText("STAGE 2");
    addAndMakeVisible(masterGroup); masterGroup.setText("MASTER");

    s1AdvBtn.addListener(this); addAndMakeVisible(s1AdvBtn);
    s2AdvBtn.addListener(this); addAndMakeVisible(s2AdvBtn);

    setSize(1000, 650);
}

MultiOtoAudioProcessorEditor::~MultiOtoAudioProcessorEditor() {
    phaseModeBox.setLookAndFeel(nullptr);
    removeAllChildren(); // Ableton Live VST3 Teardown Fail-safe
}

void MultiOtoAudioProcessorEditor::buttonClicked(juce::Button* b) {
    if (b == &s1AdvBtn) s1AdvOpen = !s1AdvOpen;
    if (b == &s2AdvBtn) s2AdvOpen = !s2AdvOpen;
    resized();
}

void MultiOtoAudioProcessorEditor::paint(juce::Graphics& g) {
    juce::ColourGradient grad(
        juce::Colour(0xFF170801), 0.0f, 0.0f,
        juce::Colour(0xFF1c0901), 0.0f, (float)getHeight(), false);
    g.setGradientFill(grad);
    g.fillAll();
}

void MultiOtoAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced(20);

    int kw = 70;
    int kh = 85;

    auto headerRow = area.removeFromTop(130);

    preDriveGroup.setBounds(headerRow.removeFromLeft(430));
    auto preArea = preDriveGroup.getBounds().reduced(15).withTrimmedTop(15);
    inGain.setBounds(preArea.removeFromLeft(kw));    preArea.removeFromLeft(10);
    drive.setBounds(preArea.removeFromLeft(kw));     preArea.removeFromLeft(10);
    oddBlend.setBounds(preArea.removeFromLeft(kw));  preArea.removeFromLeft(10);
    evenBlend.setBounds(preArea.removeFromLeft(kw)); preArea.removeFromLeft(10);
    totalOtt.setBounds(preArea.removeFromLeft(kw));

    headerRow.removeFromLeft(30);

    auto xArea = headerRow.removeFromLeft(160).withTrimmedTop(25);
    xLow.setBounds(xArea.removeFromLeft(kw));  xArea.removeFromLeft(20);
    xHigh.setBounds(xArea.removeFromLeft(kw));

    area.removeFromTop(10);

    auto rightArea = area.removeFromRight(290);
    area.removeFromRight(20);

    masterGroup.setBounds(rightArea.withHeight(280));
    auto mArea = masterGroup.getBounds().reduced(15).withTrimmedTop(20);

    auto mRow1 = mArea.removeFromTop(kh);
    mArea.removeFromTop(10);
    auto mRow2 = mArea.removeFromTop(kh);
    mArea.removeFromTop(20);
    auto mRow3 = mArea.removeFromTop(24);

    postHPF.setBounds(mRow1.removeFromLeft(kw)); mRow1.removeFromLeft(20);
    postLPF.setBounds(mRow1.removeFromLeft(kw));

    dryWet.setBounds(mRow2.removeFromLeft(kw));    mRow2.removeFromLeft(10);
    limitCeil.setBounds(mRow2.removeFromLeft(kw)); mRow2.removeFromLeft(10);
    outGain.setBounds(mRow2.removeFromLeft(kw));

    phaseModeBox.setBounds(mRow3.withSizeKeepingCentre(160, 24));

    auto layoutStage = [&](juce::GroupComponent& group, bool open,
        ArcKnob& gH, ArcKnob& gM, ArcKnob& gL,
        ArcKnob& dH, ArcKnob& dM, ArcKnob& dL, ArcKnob& time,
        ArcKnob& aH, ArcKnob& aM, ArcKnob& aL,
        ArcKnob& rH, ArcKnob& rM, ArcKnob& rL, juce::TextButton& btn,
        juce::Rectangle<int> bounds)
        {
            group.setBounds(bounds);
            btn.setBounds(bounds.getRight() - 90, bounds.getY() + 8, 80, 20);

            auto sArea = bounds.reduced(15).withTrimmedTop(20);

            auto basicArea = sArea.removeFromLeft(230);
            auto bRow1 = basicArea.removeFromTop(kh);
            basicArea.removeFromTop(10);
            auto bRow2 = basicArea.removeFromTop(kh);

            gL.setBounds(bRow1.removeFromLeft(kw)); bRow1.removeFromLeft(10);
            gM.setBounds(bRow1.removeFromLeft(kw)); bRow1.removeFromLeft(10);
            gH.setBounds(bRow1.removeFromLeft(kw));

            dL.setBounds(bRow2.removeFromLeft(kw)); bRow2.removeFromLeft(10);
            dM.setBounds(bRow2.removeFromLeft(kw)); bRow2.removeFromLeft(10);
            dH.setBounds(bRow2.removeFromLeft(kw));

            sArea.removeFromLeft(20);

            auto timeArea = sArea.removeFromLeft(kw);
            time.setBounds(timeArea.withSizeKeepingCentre(kw, kh));

            sArea.removeFromLeft(30);

            aL.setVisible(open); aM.setVisible(open); aH.setVisible(open);
            rL.setVisible(open); rM.setVisible(open); rH.setVisible(open);

            if (open) {
                auto advArea = sArea.removeFromLeft(230);
                auto aRow1 = advArea.removeFromTop(kh);
                advArea.removeFromTop(10);
                auto aRow2 = advArea.removeFromTop(kh);

                aL.setBounds(aRow1.removeFromLeft(kw)); aRow1.removeFromLeft(10);
                aM.setBounds(aRow1.removeFromLeft(kw)); aRow1.removeFromLeft(10);
                aH.setBounds(aRow1.removeFromLeft(kw));

                rL.setBounds(aRow2.removeFromLeft(kw)); aRow2.removeFromLeft(10);
                rM.setBounds(aRow2.removeFromLeft(kw)); aRow2.removeFromLeft(10);
                rH.setBounds(aRow2.removeFromLeft(kw));
            }
        };

    auto stage1Bounds = area.removeFromTop(230);
    area.removeFromTop(10);
    auto stage2Bounds = area.removeFromTop(230);

    layoutStage(stage1Group, s1AdvOpen, s1GainH, s1GainM, s1GainL, s1DepthH, s1DepthM, s1DepthL, s1Time, s1AtkH, s1AtkM, s1AtkL, s1RelH, s1RelM, s1RelL, s1AdvBtn, stage1Bounds);
    layoutStage(stage2Group, s2AdvOpen, s2GainH, s2GainM, s2GainL, s2DepthH, s2DepthM, s2DepthL, s2Time, s2AtkH, s2AtkM, s2AtkL, s2RelH, s2RelM, s2RelL, s2AdvBtn, stage2Bounds);
}