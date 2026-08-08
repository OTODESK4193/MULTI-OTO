#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ColorPalette.h"
#include "MinimalUI.h"
#include "DynVisual.h"
#include "DSP/ModMatrix.h"

struct StageMeter;

// ============================================================================
//  MainPanel — 1画面統合パネル
//
//  上段 : DynVisual (Stage1 / Stage2 横並び。帯域境界を左右ドラッグで XO 調整)
//  中段 : PRE-DRIVE / MASTER を 1 行に集約
//  下段 : Stage1 と Stage2 を左右に同時表示
//         各ステージ = TIME/MIX と LOW X/HIGH X のバー
//                    + 「3行 (LOW/MID/HIGH) × 5列 (GAIN/UP/DN/ATK/REL)」の行列
// ============================================================================
class MainPanel : public juce::Component,
                  private juce::Timer
{
public:
    MainPanel (juce::AudioProcessorValueTreeState& apvts, MultiOtoLookAndFeel& laf)
    {
        auto setupBtn = [&] (juce::TextButton& b, const char* pID,
                             std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& a,
                             juce::Colour onColour) {
            b.setClickingTogglesState (true);
            b.setColour (juce::TextButton::buttonColourId,    MOColors::knobTrack);
            b.setColour (juce::TextButton::buttonOnColourId,  onColour);
            b.setColour (juce::TextButton::textColourOffId,   MOColors::textDim);
            b.setColour (juce::TextButton::textColourOnId,    MOColors::bg);
            addAndMakeVisible (b);
            a = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, pID, b);
        };

        setupBtn (preDriveBtn, "predrive_on", preDriveAt, MOColors::accent);

        // 表示テキストのみ変更。パラメータ側の選択肢文字列は不変なので互換は保たれる。
        totalOttBox.addItemList ({ "OTT x2","OTT x4","OTT x8","OTT x16","OTT x32","OTT x64","OTT x128" }, 1);
        totalOttBox.setLookAndFeel (&laf);
        totalOttBox.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (totalOttBox);
        totalOttAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, "total_ott", totalOttBox);

        phaseModeBox.addItemList ({ "COLOR PHASE", "ALIGN PHASE" }, 1);
        phaseModeBox.setLookAndFeel (&laf);
        phaseModeBox.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (phaseModeBox);
        phaseModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, "phase_mode", phaseModeBox);

        // --- グローバル 1 行 ---
        const auto cDrive  = MOColors::accent;
        const auto cMaster = MOColors::lavender;

        inGain.build    (apvts, "in_gain",     "IN",       this, laf, cDrive);
        drive.build     (apvts, "drive",       "DRIVE",    this, laf, cDrive);
        oddBlend.build  (apvts, "odd_blend",   "ODD",      this, laf, cDrive);
        evenBlend.build (apvts, "even_blend",  "EVEN",     this, laf, cDrive);
        postHPF.build   (apvts, "post_hpf",    "HPF",      this, laf, cMaster);
        postLPF.build   (apvts, "post_lpf",    "LPF",      this, laf, cMaster);
        dryWet.build    (apvts, "dry_wet",     "DRY/WET",  this, laf, cMaster);
        outGain.build   (apvts, "out_gain",    "OUT",      this, laf, cMaster);
        limitCeil.build (apvts, "limit_ceil",  "CEILING",  this, laf, cMaster);

        // --- DynVisual ---
        dynS1.setTitle ("STAGE 1");
        dynS2.setTitle ("STAGE 2");
        dynS1.setSelected (true);
        dynS2.setSelected (true);
        addAndMakeVisible (dynS1);
        addAndMakeVisible (dynS2);

        // --- Stage ON ボタン ---
        setupBtn (s1OnBtn, "s1_on", s1OnAt, MOColors::peach);
        setupBtn (s2OnBtn, "s2_on", s2OnAt, MOColors::babyBlue);
        s1OnBtn.setButtonText ("ON");
        s2OnBtn.setButtonText ("ON");

        // --- クロスオーバー LINK (編集補助。DSP は常に各ステージの値を使う) ---
        setupBtn (xoverLinkBtn, "xover_link", xoverLinkAt, MOColors::mint);
        xoverLinkBtn.setButtonText ("LINK");
        xoverLinkBtn.setTooltip (MOText::u8 ("ON: Stage 1 の LOW X / HIGH X を動かすと Stage 2 も一緒に動きます"));

        // --- Stage 行列 ---
        buildStage (apvts, laf, 1, s1Gain, s1Up, s1Dn, s1Time, s1Mix, s1Atk, s1Rel,
                    s1XLow, s1XHigh, MOColors::peach);
        buildStage (apvts, laf, 2, s2Gain, s2Up, s2Dn, s2Time, s2Mix, s2Atk, s2Rel,
                    s2XLow, s2XHigh, MOColors::babyBlue);

        stageLabel[0].setText ("STAGE 1", juce::dontSendNotification);
        stageLabel[1].setText ("STAGE 2", juce::dontSendNotification);
        stageLabel[0].setColour (juce::Label::textColourId, MOColors::peach);
        stageLabel[1].setColour (juce::Label::textColourId, MOColors::babyBlue);
        for (auto& l : stageLabel) {
            l.setFont (juce::Font (juce::FontOptions (13.5f, juce::Font::bold)));
            addAndMakeVisible (l);
        }

        assignModDestinations();

        // LINK: Stage1 の XO を動かしたら Stage2 へ写す。
        // プリセット読込中は beginParameterBatch() で抑止されるので上書きされない。
        s1XLow.slider.onValueChange  = [this] { mirrorXoverIfLinked(); };
        s1XHigh.slider.onValueChange = [this] { mirrorXoverIfLinked(); };
        xoverLinkBtn.onClick         = [this] { mirrorXoverIfLinked(); };
    }

    /** プリセット読込など「ユーザー操作ではない」一括変更の前後で呼ぶ */
    void beginParameterBatch() { ++mirrorSuppress; }
    void endParameterBatch()
    {
        // Attachment 経由のスライダー更新が非同期で届くので、解除も後回しにする
        juce::Component::SafePointer<MainPanel> safeThis (this);
        juce::MessageManager::callAsync ([safeThis] {
            if (safeThis != nullptr) safeThis->releaseParameterBatch();
        });
    }
    void releaseParameterBatch() { mirrorSuppress = juce::jmax (0, mirrorSuppress - 1); }

    /** カラーテーマ切替時に全コントロールの色を貼り直す */
    void applyTheme()
    {
        const auto cDrive  = MOColors::accent;
        const auto cMaster = MOColors::lavender;

        inGain.setAccent (cDrive);  drive.setAccent (cDrive);
        oddBlend.setAccent (cDrive); evenBlend.setAccent (cDrive);
        postHPF.setAccent (cMaster); postLPF.setAccent (cMaster);
        dryWet.setAccent (cMaster);  outGain.setAccent (cMaster);
        limitCeil.setAccent (cMaster);

        const juce::Colour bandUp[3]   = { MOColors::bandLowUp, MOColors::bandMidUp, MOColors::bandHighUp };
        const juce::Colour bandDown[3] = { MOColors::bandLowDn, MOColors::bandMidDn, MOColors::bandHighDn };

        for (int b = 0; b < 3; ++b)
        {
            for (ArcKnob* k : { &s1Gain[b], &s1Up[b], &s1Atk[b], &s1Rel[b],
                                &s2Gain[b], &s2Up[b], &s2Atk[b], &s2Rel[b] })
                k->setAccent (bandUp[b]);

            s1Dn[b].setAccent (bandDown[b]);
            s2Dn[b].setAccent (bandDown[b]);
        }

        s1Time.setAccent (MOColors::peach);    s1Mix.setAccent (MOColors::peach);
        s2Time.setAccent (MOColors::babyBlue); s2Mix.setAccent (MOColors::babyBlue);
        s1XLow.setAccent (MOColors::babyBlue); s1XHigh.setAccent (MOColors::babyBlue);
        s2XLow.setAccent (MOColors::babyBlue); s2XHigh.setAccent (MOColors::babyBlue);

        auto reColour = [] (juce::TextButton& b, juce::Colour on) {
            b.setColour (juce::TextButton::buttonColourId,   MOColors::knobTrack);
            b.setColour (juce::TextButton::buttonOnColourId, on);
            b.setColour (juce::TextButton::textColourOffId,  MOColors::textDim);
            b.setColour (juce::TextButton::textColourOnId,   MOColors::bg);
        };
        reColour (preDriveBtn,  MOColors::accent);
        reColour (s1OnBtn,      MOColors::peach);
        reColour (s2OnBtn,      MOColors::babyBlue);
        reColour (xoverLinkBtn, MOColors::mint);

        stageLabel[0].setColour (juce::Label::textColourId, MOColors::peach);
        stageLabel[1].setColour (juce::Label::textColourId, MOColors::babyBlue);

        repaint();
    }

    ~MainPanel() override
    {
        totalOttBox.setLookAndFeel (nullptr);
        phaseModeBox.setLookAndFeel (nullptr);
    }

    void setMeters (const StageMeter* s1, const StageMeter* s2)
    {
        dynS1.setMeter (s1);
        dynS2.setMeter (s2);
    }

    /** MOD レンジ表示用。エディタから ModMatrix を差し込む。 */
    void setModMatrix (const ModMatrix* m)
    {
        matrix = m;
        if (matrix != nullptr) startTimerHz (24);
    }

    void bindApvts (juce::AudioProcessorValueTreeState& apvts)
    {
        dynS1.bindStageParameters (apvts, 1);
        dynS2.bindStageParameters (apvts, 2);
    }

    // ------------------------------------------------------------------
    void paint (juce::Graphics& g) override
    {
        MOColors::paintPanel (g, getLocalBounds());

        g.setColour (MOColors::textDim);
        g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
        g.drawText ("PRE-DRIVE / MASTER", globalLabelArea, juce::Justification::centredLeft);

        static const char* colNames[5] = { "GAIN", "UP", "DN", "ATK", "REL" };
        static const char* rowNames[3] = { "LOW", "MID", "HI" };
        // テーマで色が変わるので static にしないこと
        const juce::Colour rowCols[3] = { MOColors::bandLowUp, MOColors::bandMidUp, MOColors::bandHighUp };

        for (int s = 0; s < 2; ++s)
        {
            MOColors::paintWell (g, geom[s].block);

            g.setColour (MOColors::textDim);
            g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
            for (int c = 0; c < 5; ++c)
                g.drawText (colNames[c], geom[s].colHeader[c], juce::Justification::centred);

            g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
            for (int r = 0; r < 3; ++r) {
                g.setColour (rowCols[r]);
                g.drawText (rowNames[r], geom[s].rowHeader[r], juce::Justification::centredLeft);
            }
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10, 8);

        // --- 1. DynVisual ---
        auto dynArea = area.removeFromTop (128);
        const int halfW = (dynArea.getWidth() - 10) / 2;
        dynS1.setBounds (dynArea.removeFromLeft (halfW));
        dynArea.removeFromLeft (10);
        dynS2.setBounds (dynArea.removeFromLeft (halfW));

        area.removeFromTop (8);

        // --- 2. グローバル 1 行 ---
        globalLabelArea = area.removeFromTop (16);
        auto row = area.removeFromTop (84);

        // コンボのフォントを上げたぶん幅を確保する ("COLOR PHASE" が入る)
        auto ctrl = row.removeFromRight (118);
        preDriveBtn.setBounds  (ctrl.removeFromTop (25).reduced (2, 1));
        ctrl.removeFromTop (3);
        totalOttBox.setBounds  (ctrl.removeFromTop (25).reduced (2, 1));
        ctrl.removeFromTop (3);
        phaseModeBox.setBounds (ctrl.removeFromTop (25).reduced (2, 1));
        row.removeFromRight (6);

        ArcKnob* globals[] = { &inGain, &drive, &oddBlend, &evenBlend,
                               &postHPF, &postLPF, &dryWet, &outGain, &limitCeil };
        const int nG = (int) (sizeof (globals) / sizeof (globals[0]));
        const int cellW = row.getWidth() / nG;
        for (int i = 0; i < nG; ++i)
            globals[i]->setBounds (row.removeFromLeft (cellW));

        area.removeFromTop (8);

        // --- 3. Stage 1 / Stage 2 を左右に同時表示 ---
        const int sw = (area.getWidth() - 10) / 2;
        layoutStage (0, area.removeFromLeft (sw));
        area.removeFromLeft (10);
        layoutStage (1, area.removeFromLeft (sw));
    }

private:
    // ------------------------------------------------------------------
    /** どのノブがどの MOD 行き先かを結びつける。
        これで drawRotarySlider / drawLinearSlider が変調レンジを描けるようになる。 */
    void assignModDestinations()
    {
        auto reg = [this] (ArcKnob& k, int dst) {
            k.setModDest (dst);
            modTargets.push_back ({ &k, dst });
        };

        reg (s1Time,  ModMatrix::DstS1Time);
        reg (s2Time,  ModMatrix::DstS2Time);
        reg (s1Mix,   ModMatrix::DstS1Mix);
        reg (s2Mix,   ModMatrix::DstS2Mix);
        reg (s1XLow,  ModMatrix::DstS1XLow);
        reg (s1XHigh, ModMatrix::DstS1XHigh);
        reg (s2XLow,  ModMatrix::DstS2XLow);
        reg (s2XHigh, ModMatrix::DstS2XHigh);

        for (int b = 0; b < 3; ++b)
        {
            reg (s1Atk[b], ModMatrix::DstS1AtkL + b);
            reg (s1Rel[b], ModMatrix::DstS1RelL + b);
            reg (s2Atk[b], ModMatrix::DstS2AtkL + b);
            reg (s2Rel[b], ModMatrix::DstS2RelL + b);
        }

        lastModValue.assign (modTargets.size(), 0.0f);
    }

    /** 変調が動いている行き先だけを再描画する (全面 repaint は避ける) */
    void timerCallback() override
    {
        if (matrix == nullptr) return;

        for (size_t i = 0; i < modTargets.size(); ++i)
        {
            const int dst = modTargets[i].second;
            const float span = matrix->getRangeMaxForGui (dst) - matrix->getRangeMinForGui (dst);
            const float cur  = (span > 1.0e-4f) ? matrix->getForGui (dst) : 0.0f;

            // 変調が「今切れた」場合も 1 回だけ描き直す必要がある
            if (std::abs (cur - lastModValue[i]) > 0.002f
                || (span <= 1.0e-4f && lastModValue[i] != 0.0f))
            {
                lastModValue[i] = cur;
                modTargets[i].first->slider.repaint();
            }
        }
    }

    // ------------------------------------------------------------------
    void buildStage (juce::AudioProcessorValueTreeState& apvts, MultiOtoLookAndFeel& laf, int s,
                     ArcKnob* gn, ArcKnob* up, ArcKnob* dn, ArcKnob& tm, ArcKnob& mx,
                     ArcKnob* ak, ArcKnob* rl, ArcKnob& xlo, ArcKnob& xhi, juce::Colour macroColour)
    {
        const juce::String st (s);
        static const char* bandSuffix[3] = { "l", "m", "h" };
        static const char* bandName[3]   = { "LOW", "MID", "HI" };
        const juce::Colour bandUp[3]   = { MOColors::bandLowUp, MOColors::bandMidUp, MOColors::bandHighUp };
        const juce::Colour bandDown[3] = { MOColors::bandLowDn, MOColors::bandMidDn, MOColors::bandHighDn };

        for (int b = 0; b < 3; ++b)
        {
            const juce::String sfx (bandSuffix[b]);
            const juce::String nm (bandName[b]);

            gn[b].build (apvts, "s" + st + "_gain_" + sfx, nm + " GAIN", this, laf, bandUp[b],   false);
            up[b].build (apvts, "s" + st + "_up_"   + sfx, nm + " UP",   this, laf, bandUp[b],   false);
            dn[b].build (apvts, "s" + st + "_down_" + sfx, nm + " DN",   this, laf, bandDown[b], false);
            ak[b].build (apvts, "s" + st + "_atk_"  + sfx, nm + " ATK",  this, laf, bandUp[b],   false);
            rl[b].build (apvts, "s" + st + "_rel_"  + sfx, nm + " REL",  this, laf, bandUp[b],   false);
        }

        tm.buildBar (apvts, "s" + st + "_time", "TIME", this, laf, macroColour);
        mx.buildBar (apvts, "s" + st + "_mix",  "MIX",  this, laf, macroColour);

        // Stage 1 は旧バージョンとの互換のため従来 ID をそのまま使う
        const juce::String loID = (s == 1) ? "xover_low"  : "s2_xover_low";
        const juce::String hiID = (s == 1) ? "xover_high" : "s2_xover_high";
        xlo.buildBar (apvts, loID, "LOW X",  this, laf, MOColors::babyBlue);
        xhi.buildBar (apvts, hiID, "HIGH X", this, laf, MOColors::babyBlue);
    }

    void layoutStage (int idx, juce::Rectangle<int> b)
    {
        geom[idx].block = b;
        auto inner = b.reduced (8, 6);

        auto titleRow = inner.removeFromTop (20);
        stageLabel[idx].setBounds (titleRow.removeFromLeft (70));
        (idx == 0 ? s1OnBtn : s2OnBtn).setBounds (titleRow.removeFromRight (46).withSizeKeepingCentre (44, 18));
        if (idx == 1)
        {
            titleRow.removeFromRight (6);
            xoverLinkBtn.setBounds (titleRow.removeFromRight (52).withSizeKeepingCentre (50, 18));
        }

        inner.removeFromTop (3);
        auto macro1 = inner.removeFromTop (20);
        const int mw = (macro1.getWidth() - 8) / 2;
        (idx == 0 ? s1Time : s2Time).slider.setBounds (macro1.removeFromLeft (mw));
        macro1.removeFromLeft (8);
        (idx == 0 ? s1Mix : s2Mix).slider.setBounds (macro1.removeFromLeft (mw));

        inner.removeFromTop (3);
        auto macro2 = inner.removeFromTop (20);
        (idx == 0 ? s1XLow : s2XLow).slider.setBounds (macro2.removeFromLeft (mw));
        macro2.removeFromLeft (8);
        (idx == 0 ? s1XHigh : s2XHigh).slider.setBounds (macro2.removeFromLeft (mw));

        inner.removeFromTop (4);

        constexpr int rowLabelW = 34;
        auto colHdr = inner.removeFromTop (15);
        colHdr.removeFromLeft (rowLabelW);
        const int cw = colHdr.getWidth() / 5;
        for (int c = 0; c < 5; ++c)
            geom[idx].colHeader[c] = colHdr.removeFromLeft (cw);

        ArcKnob* gn = (idx == 0) ? s1Gain : s2Gain;
        ArcKnob* up = (idx == 0) ? s1Up   : s2Up;
        ArcKnob* dn = (idx == 0) ? s1Dn   : s2Dn;
        ArcKnob* ak = (idx == 0) ? s1Atk  : s2Atk;
        ArcKnob* rl = (idx == 0) ? s1Rel  : s2Rel;

        const int rh = inner.getHeight() / 3;
        for (int r = 0; r < 3; ++r)
        {
            auto rowR = inner.removeFromTop (rh);
            geom[idx].rowHeader[r] = rowR.removeFromLeft (rowLabelW);

            ArcKnob* cells[5] = { &gn[r], &up[r], &dn[r], &ak[r], &rl[r] };
            for (int c = 0; c < 5; ++c)
                cells[c]->setBounds (rowR.removeFromLeft (cw));
        }
    }

    // ------------------------------------------------------------------
    void mirrorXoverIfLinked()
    {
        if (mirrorSuppress > 0) return;
        if (! xoverLinkBtn.getToggleState()) return;

        s2XLow.slider.setValue  (s1XLow.slider.getValue(),  juce::sendNotificationSync);
        s2XHigh.slider.setValue (s1XHigh.slider.getValue(), juce::sendNotificationSync);
    }

    // ------------------------------------------------------------------
    struct StageGeom {
        juce::Rectangle<int> block;
        juce::Rectangle<int> colHeader[5];
        juce::Rectangle<int> rowHeader[3];
    };
    StageGeom geom[2];
    juce::Rectangle<int> globalLabelArea;

    juce::TextButton preDriveBtn { "PRE-DRIVE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> preDriveAt;

    juce::ComboBox totalOttBox, phaseModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> totalOttAttachment, phaseModeAttachment;

    ArcKnob inGain, drive, oddBlend, evenBlend;
    ArcKnob postHPF, postLPF, dryWet, outGain, limitCeil;

    DynVisualComponent dynS1, dynS2;

    juce::TextButton s1OnBtn, s2OnBtn, xoverLinkBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> s1OnAt, s2OnAt, xoverLinkAt;

    int mirrorSuppress = 0;

    const ModMatrix* matrix = nullptr;
    std::vector<std::pair<ArcKnob*, int>> modTargets;
    std::vector<float> lastModValue;

    ArcKnob s1Gain[3], s1Up[3], s1Dn[3], s1Time, s1Mix, s1Atk[3], s1Rel[3], s1XLow, s1XHigh;
    ArcKnob s2Gain[3], s2Up[3], s2Dn[3], s2Time, s2Mix, s2Atk[3], s2Rel[3], s2XLow, s2XHigh;

    juce::Label stageLabel[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};
