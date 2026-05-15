#include "MinimalUI.h"

MultiOtoLookAndFeel::MultiOtoLookAndFeel() : groupFont(juce::FontOptions(11.0f, juce::Font::bold)) {
    setColour(juce::Slider::rotarySliderFillColourId, MultiOtoColors::Accent);
    setColour(juce::Slider::rotarySliderOutlineColourId, MultiOtoColors::ArcTrack);
    setColour(juce::ComboBox::backgroundColourId, MultiOtoColors::Surface);
}

void MultiOtoLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float startAngle, float endAngle, juce::Slider&) {
    auto b = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(2.0f);
    float cx = b.getCentreX(), cy = b.getCentreY();
    // jminを使って常に正円を描画（90px四方で完全統一）
    float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.42f;
    float th = r * 0.20f;

    juce::Path track; track.addCentredArc(cx, cy, r, r, 0.0f, startAngle, endAngle, true);
    g.setColour(MultiOtoColors::ArcTrack); g.strokePath(track, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    float angle = startAngle + sliderPos * (endAngle - startAngle);
    juce::Path fill; fill.addCentredArc(cx, cy, r, r, 0.0f, startAngle, angle, true);
    g.setGradientFill(juce::ColourGradient(MultiOtoColors::AccentBlue, cx - r, cy, MultiOtoColors::Accent, cx + r, cy, false));
    g.strokePath(fill, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(MultiOtoColors::Panel); g.fillEllipse(cx - r * 0.3f, cy - r * 0.3f, r * 0.6f, r * 0.6f);
    g.setColour(MultiOtoColors::TextPrimary); g.drawLine(cx, cy, cx + r * 0.55f * std::sin(angle), cy - r * 0.55f * std::cos(angle), 1.5f);
}

void MultiOtoLookAndFeel::drawGroupComponentOutline(juce::Graphics& g, int w, int h, const juce::String&, const juce::Justification&, juce::GroupComponent&) {
    g.setColour(MultiOtoColors::Border); g.drawRoundedRectangle(0.5f, 8.0f, (float)w - 1.0f, (float)h - 9.0f, 4.0f, 1.0f);
}

void MultiOtoLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool, bool) {
    float textW = groupFont.getStringWidthFloat(b.getButtonText());
    g.setColour(juce::Colour(0xFF190802)); g.fillRect(0.0f, 0.0f, textW + 12.0f, (float)b.getHeight());
    g.setColour(b.getToggleState() ? MultiOtoColors::Accent : MultiOtoColors::TextSecondary);
    g.drawText(b.getButtonText(), 6, 0, (int)textW + 6, b.getHeight(), juce::Justification::left, true);
}

juce::Font MultiOtoLookAndFeel::getLabelFont(juce::Label&) {
    return juce::Font(juce::FontOptions(10.0f, juce::Font::bold));
}

// コンボボックスとリストのフォントを大きく太くする
juce::Font MultiOtoLookAndFeel::getComboBoxFont(juce::ComboBox&) {
    return juce::Font(juce::FontOptions(15.0f, juce::Font::bold));
}
juce::Font MultiOtoLookAndFeel::getPopupMenuFont() {
    return juce::Font(juce::FontOptions(15.0f, juce::Font::plain));
}

void ArcKnob::build(juce::AudioProcessorValueTreeState& apvts, const juce::String& pID, const juce::String& lT, juce::Component* p, MultiOtoLookAndFeel& laf) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 12);
    slider.setLookAndFeel(&laf); slider.setColour(juce::Slider::textBoxTextColourId, MultiOtoColors::TextSecondary); slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    p->addAndMakeVisible(slider); label.setText(lT, juce::dontSendNotification); label.setJustificationType(juce::Justification::centred); label.setColour(juce::Label::textColourId, MultiOtoColors::TextSecondary); p->addAndMakeVisible(label);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pID, slider);
}

void ArcKnob::setBounds(int x, int y, int w, int h) { slider.setBounds(x, y, w, h - 15); label.setBounds(x, y + h - 15, w, 15); }
void ArcKnob::setBounds(juce::Rectangle<int> r) { setBounds(r.getX(), r.getY(), r.getWidth(), r.getHeight()); }
void ArcKnob::setVisible(bool v) { slider.setVisible(v); label.setVisible(v); }