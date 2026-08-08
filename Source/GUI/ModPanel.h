// ============================================================================
//  ModPanel.h
//  MOD MATRIX オーバーレイ
//   上段 : LFO x4 (波形 / SYNC / RATE / 同期音価 / ライブ値バー)
//   下段 : 8 スロットのマトリクス (SRC -> DST, AMOUNT, UNI)
// ============================================================================
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "ColorPalette.h"
#include "MinimalUI.h"
#include "DSP/ModMatrix.h"

class ModPanel : public juce::Component,
                 private juce::Timer
{
public:
    ModPanel (juce::AudioProcessorValueTreeState& apvts, MultiOtoLookAndFeel& laf)
    {
        using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
        using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
        using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;

        auto setupCombo = [&] (juce::ComboBox& c, const juce::StringArray& items, const juce::String& id) {
            c.addItemList (items, 1);
            c.setLookAndFeel (&laf);
            c.setJustificationType (juce::Justification::centredLeft);
            addAndMakeVisible (c);
            comboAttach.push_back (std::make_unique<ComboAttach> (apvts, id, c));
        };

        // role: 0 = mint (SYNC), 1 = peach (UNI)。テーマ切替時に色を引き直すため
        // 実際の Colour ではなく役割で覚えておく。
        auto setupToggle = [&] (juce::TextButton& b, const juce::String& txt,
                                const juce::String& id, int role) {
            b.setButtonText (txt);
            b.setClickingTogglesState (true);
            addAndMakeVisible (b);
            buttonAttach.push_back (std::make_unique<ButtonAttach> (apvts, id, b));
            toggles.push_back ({ &b, role });
        };

        // ---------------- LFO x4 ----------------
        for (int i = 0; i < ModMatrix::kNumLfos; ++i)
        {
            const juce::String n (i + 1);
            const auto sz = static_cast<size_t> (i);

            setupCombo  (lfoWave[sz],     ModMatrix::getWaveNames(),     "lfo" + n + "_wave");
            setupToggle (lfoSync[sz],     "SYNC",                        "lfo" + n + "_sync", 0);
            setupCombo  (lfoSyncRate[sz], ModMatrix::getSyncRateNames(), "lfo" + n + "_syncrate");

            lfoRate[sz].setSliderStyle (juce::Slider::LinearHorizontal);
            lfoRate[sz].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            lfoRate[sz].setLookAndFeel (&laf);
            lfoRate[sz].setName ("RATE");
            lfoRate[sz].setColour (juce::Slider::rotarySliderFillColourId, MOColors::babyBlue);
            addAndMakeVisible (lfoRate[sz]);
            sliderAttach.push_back (std::make_unique<SliderAttach> (apvts, "lfo" + n + "_rate", lfoRate[sz]));

            lfoSync[sz].onClick = [this] { updateSyncVisibility(); };
        }

        // ---------------- スロット x8 ----------------
        for (int i = 0; i < ModMatrix::kNumSlots; ++i)
        {
            const juce::String n (i + 1);
            const auto sz = static_cast<size_t> (i);

            setupCombo  (slotSrc[sz], ModMatrix::getSourceNames(), "mod" + n + "_src");
            setupCombo  (slotDst[sz], ModMatrix::getDestNames(),   "mod" + n + "_dst");
            setupToggle (slotUni[sz], "UNI",                       "mod" + n + "_uni", 1);

            slotAmt[sz].setSliderStyle (juce::Slider::LinearHorizontal);
            slotAmt[sz].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            slotAmt[sz].setLookAndFeel (&laf);
            slotAmt[sz].setName ("AMT");
            slotAmt[sz].setColour (juce::Slider::rotarySliderFillColourId, MOColors::accent);
            addAndMakeVisible (slotAmt[sz]);
            sliderAttach.push_back (std::make_unique<SliderAttach> (apvts, "mod" + n + "_amt", slotAmt[sz]));
        }

        btnClose.setButtonText ("CLOSE");
        btnClose.onClick = [this] { setVisible (false); };
        addAndMakeVisible (btnClose);

        applyTheme();
        updateSyncVisibility();
        startTimerHz (30);
    }

    ~ModPanel() override
    {
        stopTimer();
        for (auto& c : lfoWave)     c.setLookAndFeel (nullptr);
        for (auto& c : lfoSyncRate) c.setLookAndFeel (nullptr);
        for (auto& c : slotSrc)     c.setLookAndFeel (nullptr);
        for (auto& c : slotDst)     c.setLookAndFeel (nullptr);
    }

    /** LFO のライブ値を読むためのソース。エディタから差し込む。 */
    void setModMatrix (const ModMatrix* m) { matrix = m; }

    void applyTheme()
    {
        for (auto& t : toggles)
        {
            t.first->setColour (juce::TextButton::buttonColourId,   MOColors::knobTrack);
            t.first->setColour (juce::TextButton::buttonOnColourId, t.second == 0 ? MOColors::mint : MOColors::peach);
            t.first->setColour (juce::TextButton::textColourOffId,  MOColors::textDim);
            t.first->setColour (juce::TextButton::textColourOnId,   MOColors::bg);
        }
        for (auto& s : lfoRate) s.setColour (juce::Slider::rotarySliderFillColourId, MOColors::babyBlue);
        for (auto& s : slotAmt) s.setColour (juce::Slider::rotarySliderFillColourId, MOColors::accent);

        btnClose.setColour (juce::TextButton::buttonColourId,  MOColors::knobTrack);
        btnClose.setColour (juce::TextButton::textColourOffId, MOColors::text);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (MOColors::bg.withAlpha (0.94f));

        auto panel = getLocalBounds().reduced (16).toFloat();
        g.setColour (MOColors::panel);
        g.fillRoundedRectangle (panel, 8.0f);
        g.setColour (MOColors::accent.withAlpha (0.65f));
        g.drawRoundedRectangle (panel, 8.0f, 1.5f);

        g.setColour (MOColors::accent);
        g.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
        g.drawText ("MOD MATRIX", titleArea, juce::Justification::centredLeft);

        g.setColour (MOColors::textDim);
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawText ("LFO SOURCES", sectionA, juce::Justification::centredLeft);
        g.drawText ("MATRIX",      sectionB, juce::Justification::centredLeft);

        // --- LFO 行のラベルとライブ値 ---
        for (int i = 0; i < ModMatrix::kNumLfos; ++i)
        {
            const auto sz = static_cast<size_t> (i);
            MOColors::paintWell (g, lfoRow[sz]);

            g.setColour (MOColors::babyBlue);
            g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
            g.drawText ("LFO " + juce::String (i + 1), lfoLabel[sz], juce::Justification::centredLeft);

            // ライブ値バー (中央 0 から左右に伸びる)
            auto b = lfoScope[sz].toFloat();
            g.setColour (MOColors::knobTrack);
            g.fillRoundedRectangle (b, 3.0f);

            const float v = juce::jlimit (-1.0f, 1.0f, lfoLive[sz]);
            const float cx = b.getCentreX();
            const float half = b.getWidth() * 0.5f - 2.0f;
            const float x0 = juce::jmin (cx, cx + v * half);
            const float wdt = std::abs (v * half);

            if (wdt > 0.5f)
            {
                g.setColour (MOColors::babyBlue.withAlpha (0.85f));
                g.fillRoundedRectangle (x0, b.getY() + 2.0f, wdt, b.getHeight() - 4.0f, 2.0f);
            }
            g.setColour (MOColors::panelLine.withAlpha (0.25f));
            g.drawVerticalLine ((int) cx, b.getY() + 1.0f, b.getBottom() - 1.0f);
        }

        // --- マトリクスの列見出し ---
        g.setColour (MOColors::textDim);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.drawText ("SOURCE",      hdrSrc, juce::Justification::centredLeft);
        g.drawText ("DESTINATION", hdrDst, juce::Justification::centredLeft);
        g.drawText ("POL",         hdrUni, juce::Justification::centredLeft);
        g.drawText ("AMOUNT",      hdrAmt, juce::Justification::centredLeft);

        // --- スロット行 ---
        for (int i = 0; i < ModMatrix::kNumSlots; ++i)
        {
            const auto sz = static_cast<size_t> (i);
            const bool active = slotSrc[sz].getSelectedItemIndex() > 0
                             && slotDst[sz].getSelectedItemIndex() > 0
                             && std::abs (slotAmt[sz].getValue()) > 0.5;

            g.setColour (active ? MOColors::accent : MOColors::textDim);
            g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
            g.drawText (juce::String (i + 1), slotNum[sz], juce::Justification::centred);
        }

        g.setColour (MOColors::textDim);
        g.setFont (MOText::bodyFont (12.5f));
        g.drawFittedText (
            MOText::u8 ("SRC を LFO / Env Follow / Random から選び、DST に送り先を指定して AMOUNT を回します。"
                        "UNI は片側 (0〜+) 振幅、OFF は両側 (−〜+) 振幅です。"
                        "LFO Rate を別の LFO で変調すると、周期そのものが不規則に揺れます。"),
            footArea, juce::Justification::topLeft, 3);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (26);

        titleArea = area.removeFromTop (28);
        btnClose.setBounds (titleArea.withTrimmedLeft (titleArea.getWidth() - 76).withSizeKeepingCentre (76, 26));
        area.removeFromTop (10);

        // ---------------- LFO ----------------
        sectionA = area.removeFromTop (16);
        area.removeFromTop (4);

        for (int i = 0; i < ModMatrix::kNumLfos; ++i)
        {
            const auto sz = static_cast<size_t> (i);
            auto row = area.removeFromTop (36);
            lfoRow[sz] = row;

            auto r = row.reduced (8, 5);
            lfoLabel[sz] = r.removeFromLeft (52);
            lfoWave[sz].setBounds (r.removeFromLeft (108).reduced (0, 1));
            r.removeFromLeft (8);
            lfoSync[sz].setBounds (r.removeFromLeft (58).reduced (0, 2));
            r.removeFromLeft (8);

            // RATE スライダーと同期音価コンボは同じ場所に重ねて置き、
            // SYNC の状態でどちらを見せるか切り替える
            auto rateArea = r.removeFromLeft (150);
            lfoRate[sz].setBounds (rateArea.reduced (0, 4));
            lfoSyncRate[sz].setBounds (rateArea.withSizeKeepingCentre (150, 24));

            r.removeFromLeft (10);
            lfoScope[sz] = r.reduced (0, 7);

            area.removeFromTop (4);
        }

        area.removeFromTop (10);

        // ---------------- マトリクス ----------------
        sectionB = area.removeFromTop (16);
        area.removeFromTop (2);

        auto hdr = area.removeFromTop (16);
        {
            auto h = hdr;
            h.removeFromLeft (26);
            hdrSrc = h.removeFromLeft (130); h.removeFromLeft (8);
            hdrDst = h.removeFromLeft (190); h.removeFromLeft (8);
            hdrUni = h.removeFromLeft (58);  h.removeFromLeft (8);
            hdrAmt = h;
        }

        footArea = area.removeFromBottom (52).withTrimmedTop (10);

        const int rowH = juce::jlimit (26, 34, area.getHeight() / ModMatrix::kNumSlots);
        for (int i = 0; i < ModMatrix::kNumSlots; ++i)
        {
            const auto sz = static_cast<size_t> (i);
            auto row = area.removeFromTop (rowH);
            auto r = row.reduced (0, 2);

            slotNum[sz] = r.removeFromLeft (26);
            slotSrc[sz].setBounds (r.removeFromLeft (130)); r.removeFromLeft (8);
            slotDst[sz].setBounds (r.removeFromLeft (190)); r.removeFromLeft (8);
            slotUni[sz].setBounds (r.removeFromLeft (58));  r.removeFromLeft (8);
            slotAmt[sz].setBounds (r.reduced (0, 3));
        }
    }

private:
    void timerCallback() override
    {
        if (matrix == nullptr) return;

        bool changed = false;
        for (int i = 0; i < ModMatrix::kNumLfos; ++i)
        {
            const float v = matrix->getLfoValue (i);
            if (std::abs (v - lfoLive[static_cast<size_t> (i)]) > 0.002f)
            {
                lfoLive[static_cast<size_t> (i)] = v;
                changed = true;
            }
        }
        if (changed) repaint();
    }

    /** SYNC の ON/OFF で RATE スライダーと音価コンボを差し替える */
    void updateSyncVisibility()
    {
        for (int i = 0; i < ModMatrix::kNumLfos; ++i)
        {
            const auto sz = static_cast<size_t> (i);
            const bool sync = lfoSync[sz].getToggleState();
            lfoRate[sz].setVisible (! sync);
            lfoSyncRate[sz].setVisible (sync);
        }
    }

    const ModMatrix* matrix = nullptr;

    std::array<juce::ComboBox,  ModMatrix::kNumLfos> lfoWave, lfoSyncRate;
    std::array<juce::TextButton, ModMatrix::kNumLfos> lfoSync;
    std::array<juce::Slider,    ModMatrix::kNumLfos> lfoRate;
    std::array<float,           ModMatrix::kNumLfos> lfoLive { };

    std::array<juce::ComboBox,   ModMatrix::kNumSlots> slotSrc, slotDst;
    std::array<juce::TextButton, ModMatrix::kNumSlots> slotUni;
    std::array<juce::Slider,     ModMatrix::kNumSlots> slotAmt;

    juce::TextButton btnClose;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttach;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>   sliderAttach;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>   buttonAttach;
    std::vector<std::pair<juce::TextButton*, int>> toggles;   // int = 色の役割

    juce::Rectangle<int> titleArea, sectionA, sectionB, footArea;
    juce::Rectangle<int> hdrSrc, hdrDst, hdrUni, hdrAmt;
    std::array<juce::Rectangle<int>, ModMatrix::kNumLfos>  lfoRow, lfoLabel, lfoScope;
    std::array<juce::Rectangle<int>, ModMatrix::kNumSlots> slotNum;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModPanel)
};
