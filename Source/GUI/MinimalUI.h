#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ColorPalette.h"
#include "DSP/ModMatrix.h"

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

    /** カラーテーマ切替後に LookAndFeel 側の色を貼り直す */
    void refreshColours();

    /** MOD レンジ表示に使う。スライダー側は getProperties()["modDst"] で行き先を持つ。 */
    void setModMatrix (const ModMatrix* m) { modMatrix = m; }

private:
    /** そのスライダーに掛かっている変調の下限・上限を「値」で返す。
        変調が無ければ false。 */
    bool getModSpan (juce::Slider& s, double& loOut, double& hiOut, double& curOut) const;

    const ModMatrix* modMatrix = nullptr;
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

    /** テーマ切替時にアクセント色を貼り直す */
    void setAccent(juce::Colour c);

    /** MOD の行き先 (ModMatrix::Dst)。設定すると変調レンジが描画される。 */
    void setModDest(int dst) { slider.getProperties().set("modDst", dst); }
};
