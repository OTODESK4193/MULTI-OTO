#include "MinimalUI.h"

MultiOtoLookAndFeel::MultiOtoLookAndFeel() {
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

    juce::Path track;
    track.addCentredArc(cx, cy, r, r, 0.0f, startAngle, endAngle, true);
    g.setColour(MultiOtoColors::ArcTrack);
    g.strokePath(track, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    float angle = startAngle + sliderPos * (endAngle - startAngle);
    juce::Path fill;
    fill.addCentredArc(cx, cy, r, r, 0.0f, startAngle, angle, true);

    juce::ColourGradient grad(MultiOtoColors::AccentBlue, cx - r, cy, MultiOtoColors::Accent, cx + r, cy, false);
    g.setGradientFill(grad);
    g.strokePath(fill, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(MultiOtoColors::Panel);
    g.fillEllipse(cx - r * 0.3f, cy - r * 0.3f, r * 0.6f, r * 0.6f);

    float ix = cx + r * 0.55f * std::sin(angle);
    float iy = cy - r * 0.55f * std::cos(angle);
    g.setColour(MultiOtoColors::TextPrimary);
    g.drawLine(cx, cy, ix, iy, 1.5f);
}

void MultiOtoLookAndFeel::drawGroupComponentOutline(juce::Graphics& g, int w, int h, const juce::String& text, const juce::Justification&, juce::GroupComponent&) {
    // 枠線の描画
    g.setColour(MultiOtoColors::Border);
    g.drawRoundedRectangle(0.5f, 8.0f, (float)w - 1.0f, (float)h - 9.0f, 4.0f, 1.0f);

    juce::Font font(juce::FontOptions(11.0f, juce::Font::bold));
    g.setFont(font);
    int textW = font.getStringWidth(text);

    // 【修正】枠線が文字を貫通しないよう、文字の裏側の背景をベースの茶色で塗りつぶす
    g.setColour(juce::Colour(0xFF261810));
    g.fillRect(8, 0, textW + 12, 16);

    // 文字の描画
    g.setColour(MultiOtoColors::TextSecondary);
    g.drawText(text, 14, 0, textW, 16, juce::Justification::left);
}