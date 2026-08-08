// ============================================================================
//  ModDestSelector.h
//  MOD マトリクスの Destination 選択コントロール。
//  クリックすると STAGE 1 / STAGE 2 / LFO RATE に分類されたツリー状の
//  PopupMenu を出す。行き先が 30 個あるため、素の ComboBox では長すぎる。
//
//  【重要】APVTS::getParameterAsValue() が返す juce::Value は使わないこと。
//  あの Value は APVTS 内部 ValueTree の子ノードを直接参照しているため、
//    apvts.replaceState()  (プリセット読込 / DAW セッション復元)
//  でツリーが差し替わると孤立した古いノードを掴んだままになり、
//    ・プリセットを読んでもアサイン先が None のまま更新されない
//    ・以後この UI から値を変えてもパラメータに届かない
//  という不具合になる。Slider/ComboBox/Button の Attachment は
//  「パラメータ本体」を購読していて影響を受けないので、ここも
//  juce::ParameterAttachment に揃える。
// ============================================================================
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include "ColorPalette.h"
#include "DSP/ModMatrix.h"

class ModDestSelector : public juce::Component
{
public:
    ModDestSelector() = default;
    ~ModDestSelector() override = default;

    void bindTo (juce::AudioProcessorValueTreeState& state, const juce::String& paramID)
    {
        attachment.reset();
        param = state.getParameter (paramID);

        if (param == nullptr)
        {
            jassertfalse;   // パラメータ ID の綴り間違い
            return;
        }

        attachment = std::make_unique<juce::ParameterAttachment> (
            *param,
            [this] (float newDenormalisedValue) { updateFromValue (newDenormalisedValue); },
            nullptr);

        attachment->sendInitialUpdate();
    }

    int getCurrentDst() const noexcept { return currentDst; }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        g.setColour (MOColors::knobTrack);
        g.fillRoundedRectangle (b, 3.0f);
        g.setColour (isMouseOver() ? MOColors::accent
                                   : MOColors::panelLine.withAlpha (0.18f));
        g.drawRoundedRectangle (b.reduced (0.5f), 3.0f, 1.0f);

        auto textArea = b.reduced (8.0f, 0.0f);
        textArea.removeFromRight (16.0f);

        g.setColour (currentDst > 0 ? MOColors::text : MOColors::textDim);
        g.setFont (juce::Font (juce::FontOptions (13.5f, juce::Font::bold)));
        g.drawText (currentText, textArea, juce::Justification::centredLeft, true);

        // ComboBox と同じ見た目の下向き矢印 (操作感を揃える)
        g.setColour (MOColors::textDim);
        const float ax = b.getRight() - 12.0f;
        const float ay = b.getCentreY();
        juce::Path arrow;
        arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
        g.fillPath (arrow);
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { repaint(); }

    void mouseDown (const juce::MouseEvent&) override
    {
        if (attachment == nullptr) return;

        juce::Component::SafePointer<ModDestSelector> safeThis (this);

        buildMenu().showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
            [safeThis] (int result)
            {
                if (safeThis == nullptr || result <= 0) return;
                if (safeThis->attachment == nullptr) return;

                // itemID = Dst + 1 (0 は PopupMenu の「選択せず閉じた」予約値)
                safeThis->attachment->setValueAsCompleteGesture (static_cast<float> (result - 1));
            });
    }

private:
    juce::PopupMenu buildMenu() const
    {
        const auto names = ModMatrix::getDestNames();
        juce::PopupMenu menu;

        auto add = [&] (juce::PopupMenu& m, int dst)
        {
            if (dst >= 0 && dst < names.size())
                m.addItem (dst + 1, names[dst], true, dst == currentDst);
        };

        menu.addItem (static_cast<int> (ModMatrix::DstNone) + 1, "None",
                      true, currentDst == static_cast<int> (ModMatrix::DstNone));
        menu.addSeparator();

        // --- STAGE 1 / STAGE 2 ---
        for (int st = 0; st < 2; ++st)
        {
            juce::PopupMenu sub;

            sub.addSectionHeader ("Macro");
            add (sub, ModMatrix::DstS1Time + st);
            add (sub, ModMatrix::DstS1Mix  + st);

            sub.addSectionHeader ("Crossover");
            add (sub, ModMatrix::DstS1XLow  + st * 2);
            add (sub, ModMatrix::DstS1XHigh + st * 2);

            sub.addSectionHeader ("Band Gain");
            for (int b = 0; b < 3; ++b) add (sub, ModMatrix::DstS1GainL + st * 3 + b);

            sub.addSectionHeader ("Attack");
            for (int b = 0; b < 3; ++b) add (sub, ModMatrix::DstS1AtkL + st * 6 + b);

            sub.addSectionHeader ("Release");
            for (int b = 0; b < 3; ++b) add (sub, ModMatrix::DstS1RelL + st * 6 + b);

            menu.addSubMenu ("STAGE " + juce::String (st + 1), sub);
        }

        // --- LFO RATE (クロスモジュレーション) ---
        {
            juce::PopupMenu sub;
            for (int i = 0; i < ModMatrix::kNumLfos; ++i)
                add (sub, ModMatrix::DstLfo1Rate + i);
            menu.addSubMenu ("LFO RATE", sub);
        }

        return menu;
    }

    void updateFromValue (float denormalisedValue)
    {
        // 非正規化値は float の丸め誤差で整数ちょうどにならないことがある。
        // 切り捨てるとアサイン先が 1 つ手前へずれるので必ず四捨五入する。
        const int dst = juce::roundToInt (denormalisedValue);

        // オートメーションで頻繁に呼ばれ得るので毎回の StringArray 生成は避ける
        static const auto names = ModMatrix::getDestNames();
        currentDst  = (dst >= 0 && dst < names.size()) ? dst : 0;
        currentText = names[currentDst];
        repaint();
    }

    juce::RangedAudioParameter* param = nullptr;
    std::unique_ptr<juce::ParameterAttachment> attachment;

    int currentDst = 0;
    juce::String currentText { "None" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModDestSelector)
};
