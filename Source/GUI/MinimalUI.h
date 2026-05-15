#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace MultiOtoColors {
    const juce::Colour Background{ 0xFF121212 };
    const juce::Colour Surface{ 0xFF1A1A1A };
    const juce::Colour Panel{ 0xFF242424 };
    const juce::Colour Border{ 0xFF333333 };
    const juce::Colour Accent{ 0xFFFF6B00 };
    const juce::Colour AccentBlue{ 0xFF00A3FF };
    const juce::Colour TextPrimary{ 0xFFE0E0E0 };
    const juce::Colour TextSecondary{ 0xFF777777 };
    const juce::Colour ArcTrack{ 0xFF2A2A2A };
}

class MultiOtoLookAndFeel : public juce::LookAndFeel_V4 {
public:
    MultiOtoLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float startAngle, float endAngle, juce::Slider& slider) override;
    void drawGroupComponentOutline(juce::Graphics& g, int w, int h, const juce::String& text, const juce::Justification& justification, juce::GroupComponent& group) override;
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    juce::Font getLabelFont(juce::Label&) override;

private:
    juce::Font groupFont;
};

struct ArcKnob {
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void build(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID, const juce::String& labelText, juce::Component* parent, MultiOtoLookAndFeel& laf);
    void setBounds(int x, int y, int w, int h);
    void setBounds(juce::Rectangle<int> rect);
    void setVisible(bool v);
};