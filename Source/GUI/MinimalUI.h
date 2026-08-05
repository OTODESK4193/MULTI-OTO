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
// ============================================================================
class MultiOtoLookAndFeel : public juce::LookAndFeel_V4 {
public:
    MultiOtoLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float startAngle, float endAngle, juce::Slider& slider) override;
    void drawGroupComponentOutline(juce::Graphics& g, int w, int h, const juce::String& text, const juce::Justification& justification, juce::GroupComponent& group) override;
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    juce::Font getLabelFont(juce::Label&) override;

    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;

private:
    juce::Font groupFont;
};

// ============================================================================
//  ArcKnob — スライダー + ラベルのヘルパー
// ============================================================================
struct ArcKnob {
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void build(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID, const juce::String& labelText, juce::Component* parent, MultiOtoLookAndFeel& laf);
    void setBounds(int x, int y, int w, int h);
    void setBounds(juce::Rectangle<int> rect);
    void setVisible(bool v);
};