#include "MinimalUI.h"

MultiOtoLookAndFeel::MultiOtoLookAndFeel() : groupFont(juce::FontOptions(11.0f, juce::Font::bold)) {
    setColour(juce::Slider::rotarySliderFillColourId, MultiOtoColors::Accent);
    setColour(juce::Slider::rotarySliderOutlineColourId, MultiOtoColors::ArcTrack);
    setColour(juce::ComboBox::backgroundColourId, MultiOtoColors::Surface);
}

void MultiOtoLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float startAngle, float endAngle, juce::Slider& slider) {
    juce::ignoreUnused(slider);
    auto b = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(2.0f);
    float cx = b.getCentreX(), cy = b.getCentreY();
    float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.42f;
    float th = r * 0.20f;

    juce::Path track; track.addCentredArc(cx, cy, r, r, 0.0f, startAngle, endAngle, true);
    g.setColour(MultiOtoColors::ArcTrack);
    g.strokePath(track, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    float angle = startAngle + sliderPos * (endAngle - startAngle);
    juce::Path fill; fill.addCentredArc(cx, cy, r, r, 0.0f, startAngle, angle, true);
    juce::ColourGradient grad(MultiOtoColors::AccentBlue, cx - r, cy, MultiOtoColors::Accent, cx + r, cy, false);
    g.setGradientFill(grad);
    g.strokePath(fill, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(MultiOtoColors::Panel); g.fillEllipse(cx - r * 0.3f, cy - r * 0.3f, r * 0.6f, r * 0.6f);
    float ix = cx + r * 0.55f * std::sin(angle), iy = cy - r * 0.55f * std::cos(angle);
    g.setColour(MultiOtoColors::TextPrimary); g.drawLine(cx, cy, ix, iy, 1.5f);
}

void MultiOtoLookAndFeel::drawGroupComponentOutline(juce::Graphics& g, int w, int h, const juce::String&, const juce::Justification&, juce::GroupComponent&) {
    g.setColour(MultiOtoColors::Border);
    g.drawRoundedRectangle(0.5f, 8.0f, (float)w - 1.0f, (float)h - 9.0f, 4.0f, 1.0f);
}

void MultiOtoLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool, bool) {
    g.setFont(groupFont);
    // 【修正】JUCE 8.0 非推奨エラー対策として getStringWidthFloat を使用
    float textW = groupFont.getStringWidthFloat(button.getButtonText());

    g.setColour(juce::Colour(0xFF190802));
    g.fillRect(0.0f, 0.0f, textW + 12.0f, (float)button.getHeight());

    if (button.getToggleState()) g.setColour(MultiOtoColors::Accent);
    else g.setColour(MultiOtoColors::TextSecondary);

    g.drawText(button.getButtonText(), 6, 0, (int)textW + 6, button.getHeight(), juce::Justification::left, true);
}

juce::Font MultiOtoLookAndFeel::getLabelFont(juce::Label&) {
    return juce::Font(juce::FontOptions(10.0f, juce::Font::bold));
}

void ArcKnob::build(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID, const juce::String& labelText, juce::Component* parent, MultiOtoLookAndFeel& laf) {
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

void ArcKnob::setBounds(int x, int y, int w, int h) {
    slider.setBounds(x, y, w, h - 15);
    label.setBounds(x, y + h - 15, w, 15);
}

void ArcKnob::setBounds(juce::Rectangle<int> rect) {
    setBounds(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

void ArcKnob::setVisible(bool v) {
    slider.setVisible(v);
    label.setVisible(v);
}