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
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float sliderPos, float startAngle, float endAngle, juce::Slider&) override;
    void drawGroupComponentOutline(juce::Graphics&, int w, int h, const juce::String&, const juce::Justification&, juce::GroupComponent&) override;
    juce::Font getLabelFont(juce::Label&) override { return juce::Font(juce::FontOptions(10.0f, juce::Font::bold)); }
};

struct ArcKnob {
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void build(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID, const juce::String& labelText, juce::Component* parent, MultiOtoLookAndFeel& laf) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 12);
        slider.setLookAndFeel(&laf);
        slider.setColour(juce::Slider::textBoxTextColourId, MultiOtoColors::TextSecondary);
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        parent->addAndMakeVisible(slider);

        label.setText(labelText, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, MultiOtoColors::TextSecondary);
        parent->addAndMakeVisible(label);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramID, slider);
    }

    void setBounds(int x, int y, int w, int h) {
        slider.setBounds(x, y, w, h - 15);
        label.setBounds(x, y + h - 15, w, 15);
    }

    // juce::Rectangleを受け取るオーバーロードを追加
    void setBounds(juce::Rectangle<int> rect) {
        setBounds(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    }

    void setVisible(bool v) {
        slider.setVisible(v);
        label.setVisible(v);
    }
};