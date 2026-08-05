#include "MinimalUI.h"

MultiOtoLookAndFeel::MultiOtoLookAndFeel() : groupFont(juce::FontOptions(12.0f, juce::Font::bold)) {
    setColour(juce::Slider::rotarySliderFillColourId, MOColors::accent);
    setColour(juce::Slider::rotarySliderOutlineColourId, MOColors::knobTrack);
    setColour(juce::ComboBox::backgroundColourId, MOColors::panel);
    setColour(juce::ComboBox::textColourId, MOColors::text);
    setColour(juce::ComboBox::outlineColourId, MOColors::panelLine.withAlpha(0.15f));
    setColour(juce::PopupMenu::backgroundColourId, MOColors::panel);
    setColour(juce::PopupMenu::textColourId, MOColors::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, MOColors::accent.withAlpha(0.3f));
}

void MultiOtoLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float startAngle, float endAngle, juce::Slider&) {
    auto b = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(2.0f);
    float cx = b.getCentreX(), cy = b.getCentreY();
    float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.44f;
    float th = r * 0.22f;

    // トラック
    juce::Path track; track.addCentredArc(cx, cy, r, r, 0.0f, startAngle, endAngle, true);
    g.setColour(MOColors::knobTrack);
    g.strokePath(track, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 値アーク (グラデーション)
    float angle = startAngle + sliderPos * (endAngle - startAngle);
    juce::Path fill; fill.addCentredArc(cx, cy, r, r, 0.0f, startAngle, angle, true);
    g.setGradientFill(juce::ColourGradient(MOColors::babyBlue, cx - r, cy, MOColors::accent, cx + r, cy, false));
    g.strokePath(fill, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 外側グロー
    if (sliderPos > 0.01f)
    {
        juce::Path glow; glow.addCentredArc(cx, cy, r + 2.5f, r + 2.5f, 0.0f, startAngle, angle, true);
        g.setColour(MOColors::accent.withAlpha(0.12f));
        g.strokePath(glow, juce::PathStrokeType(th + 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 中心キャップ
    g.setColour(MOColors::panel);
    g.fillEllipse(cx - r * 0.35f, cy - r * 0.35f, r * 0.7f, r * 0.7f);

    // インジケーター線
    g.setColour(MOColors::text);
    g.drawLine(cx, cy, cx + r * 0.6f * std::sin(angle), cy - r * 0.6f * std::cos(angle), 2.0f);
}

void MultiOtoLookAndFeel::drawGroupComponentOutline(juce::Graphics& g, int w, int h, const juce::String&, const juce::Justification&, juce::GroupComponent&) {
    g.setColour(MOColors::panelLine.withAlpha(0.12f));
    g.drawRoundedRectangle(0.5f, 8.0f, (float)w - 1.0f, (float)h - 9.0f, 4.0f, 1.0f);
}

void MultiOtoLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool, bool) {
    float textW = groupFont.getStringWidthFloat(b.getButtonText());
    g.setColour(MOColors::bg);
    g.fillRect(0.0f, 0.0f, textW + 12.0f, (float)b.getHeight());
    g.setColour(b.getToggleState() ? MOColors::accent : MOColors::textDim);
    g.drawText(b.getButtonText(), 6, 0, (int)textW + 6, b.getHeight(), juce::Justification::left, true);
}

juce::Font MultiOtoLookAndFeel::getLabelFont(juce::Label&) {
    return juce::Font(juce::FontOptions(11.0f, juce::Font::bold));
}

juce::Font MultiOtoLookAndFeel::getComboBoxFont(juce::ComboBox&) {
    return juce::Font(juce::FontOptions(13.0f, juce::Font::bold));
}

juce::Font MultiOtoLookAndFeel::getPopupMenuFont() {
    return juce::Font(juce::FontOptions(13.0f, juce::Font::plain));
}

void ArcKnob::build(juce::AudioProcessorValueTreeState& apvts, const juce::String& pID, const juce::String& lT, juce::Component* p, MultiOtoLookAndFeel& laf) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 65, 14);
    slider.setLookAndFeel(&laf);
    slider.setColour(juce::Slider::textBoxTextColourId, MOColors::text);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    p->addAndMakeVisible(slider);

    label.setText(lT, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, MOColors::textDim);
    p->addAndMakeVisible(label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pID, slider);
}

void ArcKnob::setBounds(int x, int y, int w, int h) {
    slider.setBounds(x, y, w, h - 16);
    label.setBounds(x - 5, y + h - 16, w + 10, 16);  // ラベル切れ防止のため左右に5pxマージン拡張
}
void ArcKnob::setBounds(juce::Rectangle<int> r) { setBounds(r.getX(), r.getY(), r.getWidth(), r.getHeight()); }
void ArcKnob::setVisible(bool v) { slider.setVisible(v); label.setVisible(v); }