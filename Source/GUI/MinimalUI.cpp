#include "MinimalUI.h"
#include <cmath>

MultiOtoLookAndFeel::MultiOtoLookAndFeel() : groupFont(juce::FontOptions(12.0f, juce::Font::bold)) {
    setColour(juce::Slider::rotarySliderFillColourId, MOColors::accent);
    setColour(juce::Slider::rotarySliderOutlineColourId, MOColors::knobTrack);
    setColour(juce::ComboBox::backgroundColourId, MOColors::knobTrack);
    setColour(juce::ComboBox::textColourId, MOColors::text);
    setColour(juce::ComboBox::arrowColourId, MOColors::textDim);
    setColour(juce::ComboBox::outlineColourId, MOColors::panelLine.withAlpha(0.18f));
    setColour(juce::PopupMenu::backgroundColourId, MOColors::panel);
    setColour(juce::PopupMenu::textColourId, MOColors::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, MOColors::accent.withAlpha(0.35f));
}

juce::String MultiOtoLookAndFeel::formatValue(double v) {
    const double a = std::abs(v);
    if (a >= 1000.0) return juce::String(juce::roundToInt(v / 10.0) * 10);
    if (a >= 100.0)  return juce::String(juce::roundToInt(v));
    return juce::String(v, 1);
}

void MultiOtoLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                           float sliderPos, float startAngle, float endAngle,
                                           juce::Slider& slider) {
    auto b = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(2.0f);
    const float cx = b.getCentreX(), cy = b.getCentreY();
    const float r  = juce::jmin(b.getWidth(), b.getHeight()) * 0.46f;
    const float th = juce::jmax(4.0f, r * 0.23f);

    const juce::Colour accent = slider.findColour(juce::Slider::rotarySliderFillColourId);
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    // トラック
    juce::Path track; track.addCentredArc(cx, cy, r, r, 0.0f, startAngle, endAngle, true);
    g.setColour(MOColors::knobTrack);
    g.strokePath(track, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 値アーク
    if (sliderPos > 0.002f) {
        juce::Path fill; fill.addCentredArc(cx, cy, r, r, 0.0f, startAngle, angle, true);
        g.setColour(accent);
        g.strokePath(fill, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 中心キャップ
    const float capR = juce::jmax(6.0f, r - th * 0.70f);
    g.setColour(MOColors::well);
    g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

    // ハンドル (現在位置のドット)
    const float hx = cx + std::sin(angle) * r;
    const float hy = cy - std::cos(angle) * r;
    g.setColour(MOColors::text);
    g.fillEllipse(hx - th * 0.30f, hy - th * 0.30f, th * 0.60f, th * 0.60f);

    // 中心に数値
    const float fontH = juce::jlimit(9.0f, 15.0f, capR * 0.80f);
    g.setColour(slider.isEnabled() ? MOColors::text : MOColors::textDim);
    g.setFont(juce::Font(juce::FontOptions(fontH, juce::Font::bold)));
    g.drawText(formatValue(slider.getValue()),
               juce::Rectangle<float>(cx - capR, cy - fontH * 0.75f, capR * 2.0f, fontH * 1.5f),
               juce::Justification::centred, false);
}

void MultiOtoLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                                           float, float, float,
                                           juce::Slider::SliderStyle, juce::Slider& slider) {
    auto b = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h);
    const juce::Colour accent = slider.findColour(juce::Slider::rotarySliderFillColourId);

    g.setColour(MOColors::knobTrack);
    g.fillRoundedRectangle(b, 3.0f);

    const float prop = (float)slider.valueToProportionOfLength(slider.getValue());
    const float fw = b.getWidth() * juce::jlimit(0.0f, 1.0f, prop);
    if (fw > 2.0f) {
        g.setColour(accent.withAlpha(0.60f));
        g.fillRoundedRectangle(b.withWidth(fw), 3.0f);
    }

    // 文字は「塗り部分」と「未塗り部分」でクリップを分けて 2 回描く。
    // 明るいバーの上に明るい文字が乗って消える問題を根本から回避する。
    const auto textArea = b.reduced(7.0f, 0.0f);
    const juce::String nameTxt = slider.getName();
    const juce::String valTxt  = formatValue(slider.getValue());

    auto drawPair = [&](juce::Colour nameCol, juce::Colour valCol) {
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.setColour(nameCol);
        g.drawText(nameTxt, textArea, juce::Justification::centredLeft, false);
        g.setColour(valCol);
        g.drawText(valTxt, textArea, juce::Justification::centredRight, false);
    };

    // 塗られている領域 → 暗い文字
    if (fw > 0.5f) {
        g.saveState();
        g.reduceClipRegion(b.withWidth(fw).getSmallestIntegerContainer());
        drawPair(MOColors::bg.withAlpha(0.75f), MOColors::bg);
        g.restoreState();
    }

    // 塗られていない領域 → 明るい文字
    if (fw < b.getWidth() - 0.5f) {
        g.saveState();
        g.reduceClipRegion(b.withTrimmedLeft(fw).getSmallestIntegerContainer());
        drawPair(MOColors::textDim, MOColors::text);
        g.restoreState();
    }
}

void MultiOtoLookAndFeel::drawGroupComponentOutline(juce::Graphics& g, int w, int h, const juce::String&,
                                                    const juce::Justification&, juce::GroupComponent&) {
    g.setColour(MOColors::panelLine.withAlpha(0.12f));
    g.drawRoundedRectangle(0.5f, 8.0f, (float)w - 1.0f, (float)h - 9.0f, 4.0f, 1.0f);
}

void MultiOtoLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool, bool) {
    const float textW = groupFont.getStringWidthFloat(b.getButtonText());
    g.setColour(MOColors::bg);
    g.fillRect(0.0f, 0.0f, textW + 14.0f, (float)b.getHeight());
    g.setColour(b.getToggleState() ? MOColors::accent : MOColors::textDim);
    g.drawText(b.getButtonText(), 6, 0, (int)textW + 8, b.getHeight(), juce::Justification::left, true);
}

juce::Font MultiOtoLookAndFeel::getLabelFont(juce::Label&) {
    return juce::Font(juce::FontOptions(10.5f, juce::Font::bold));
}

juce::Font MultiOtoLookAndFeel::getComboBoxFont(juce::ComboBox&) {
    return juce::Font(juce::FontOptions(11.5f, juce::Font::bold));
}

juce::Font MultiOtoLookAndFeel::getPopupMenuFont() {
    return juce::Font(juce::FontOptions(12.5f, juce::Font::plain));
}

// ============================================================================
//  ArcKnob
// ============================================================================
void ArcKnob::build(juce::AudioProcessorValueTreeState& apvts, const juce::String& pID,
                    const juce::String& lT, juce::Component* p, MultiOtoLookAndFeel& laf,
                    juce::Colour accent, bool showLabel) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);   // 値はノブ中心に描画
    slider.setLookAndFeel(&laf);
    slider.setName(lT);
    slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, MOColors::knobTrack);
    slider.setTooltip(lT);
    p->addAndMakeVisible(slider);

    hasLabel = showLabel;
    if (showLabel) {
        label.setText(lT, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, MOColors::textDim);
        label.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        label.setMinimumHorizontalScale(0.75f);
        label.setInterceptsMouseClicks(false, false);
        p->addAndMakeVisible(label);
    }

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pID, slider);
}

void ArcKnob::buildBar(juce::AudioProcessorValueTreeState& apvts, const juce::String& pID,
                       const juce::String& name, juce::Component* p, MultiOtoLookAndFeel& laf,
                       juce::Colour accent) {
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel(&laf);
    slider.setName(name);
    slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
    slider.setTooltip(name);
    p->addAndMakeVisible(slider);

    hasLabel = false;
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pID, slider);
}

void ArcKnob::setBounds(juce::Rectangle<int> r) {
    if (hasLabel) {
        auto lab = r.removeFromBottom(13);
        label.setBounds(lab.expanded(6, 0));
    }
    slider.setBounds(r);
}

void ArcKnob::setVisible(bool v) {
    slider.setVisible(v);
    if (hasLabel) label.setVisible(v);
}
