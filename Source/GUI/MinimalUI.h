#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ColorPalette.h"

// ============================================================================
//  MultiOtoColors — 後方互換用エイリアス (新規コードは MOColors を使用)
// ============================================================================
namespace MultiOtoColors {
    inline const juce::Colour Background     = MOColors::bg;
    inline const juce::Colour Surface        = MOColors::panel;
    inline const juce::Colour Panel          = MOColors::panel;
    inline const juce::Colour Border         = MOColors::panelLine.withAlpha (0.13f);
    inline const juce::Colour Accent         = MOColors::accent;
    inline const juce::Colour AccentBlue     = MOColors::babyBlue;
    inline const juce::Colour TextPrimary    = MOColors::text;
    inline const juce::Colour TextSecondary  = MOColors::textDim;
    inline const juce::Colour ArcTrack       = MOColors::knobTrack;
}

// ============================================================================
//  MultiOtoLookAndFeel
//  ノブは TextBox を持たず、数値を中心キャップに直接描画します。
//  これにより同じセル面積でノブ直径を約 2 倍にできます。
// ============================================================================
class MultiOtoLookAndFeel : public juce::LookAndFeel_V4 {
public:
    MultiOtoLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawGroupComponentOutline(juce::Graphics& g, int w, int h, const juce::String& text,
                                   const juce::Justification& justification, juce::GroupComponent& group) override;
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;

    /** スライダー値を短い文字列にする (ノブ中心表示用) */
    static juce::String formatValue(double v);

private:
    juce::Font groupFont;
};

// ============================================================================
//  ArcKnob — ロータリーノブ (+ 任意のラベル)
// ============================================================================
struct ArcKnob {
    juce::Slider slider;
    juce::Label  label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    bool hasLabel = true;

    /** 回転ノブとして構築。showLabel=false なら下部ラベルを作らない (行列レイアウト用)。 */
    void build(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
               const juce::String& labelText, juce::Component* parent, MultiOtoLookAndFeel& laf,
               juce::Colour accent = MOColors::accent, bool showLabel = true);

    /** 横バー (マクロ用) として構築。名前と値がバー内に描かれる。 */
    void buildBar(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
                  const juce::String& name, juce::Component* parent, MultiOtoLookAndFeel& laf,
                  juce::Colour accent = MOColors::accent);

    void setBounds(juce::Rectangle<int> rect);
    void setVisible(bool v);
};
