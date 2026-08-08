#include "MinimalUI.h"
#include <cmath>

MultiOtoLookAndFeel::MultiOtoLookAndFeel() : groupFont(juce::FontOptions(12.0f, juce::Font::bold)) {
    refreshColours();
}

void MultiOtoLookAndFeel::refreshColours() {
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

// ============================================================================
//  MOD レンジ算出
//  変調量 (-1..+1) を実際の値へ展開し、ノブ上での「振れ幅」を求める。
//  ModMatrix::applyModToValue を使うので、表示と実音の倍率は必ず一致する。
// ============================================================================
bool MultiOtoLookAndFeel::getModSpan(juce::Slider& s, double& loOut, double& hiOut, double& curOut) const {
    if (modMatrix == nullptr) return false;

    const int dst = static_cast<int>(s.getProperties().getWithDefault("modDst", 0));
    if (dst <= 0) return false;

    const float rLo = modMatrix->getRangeMinForGui(dst);
    const float rHi = modMatrix->getRangeMaxForGui(dst);
    if (rHi - rLo < 1.0e-4f) return false;   // 何も割り当たっていない

    const double base = s.getValue();
    const double mn = s.getMinimum(), mx = s.getMaximum();

    loOut  = juce::jlimit(mn, mx, modMatrix->applyModToValue(dst, base, rLo));
    hiOut  = juce::jlimit(mn, mx, modMatrix->applyModToValue(dst, base, rHi));
    curOut = juce::jlimit(mn, mx, modMatrix->applyModToValue(dst, base, modMatrix->getForGui(dst)));
    return true;
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

    // --- MOD レンジ (帯) ---
    // リングの外側に別の輪を足すと「浮いた」見た目になるので、
    // 値アークと「同じリングの上」を塗り分ける方式にしている。
    double mLo, mHi, mCur;
    const bool hasMod = getModSpan(slider, mLo, mHi, mCur);
    float aModCur = 0.0f;

    if (hasMod) {
        auto toAngle = [&](double v) {
            return startAngle + (float)slider.valueToProportionOfLength(v) * (endAngle - startAngle);
            };
        float aLo = toAngle(juce::jmin(mLo, mHi));
        float aHi = toAngle(juce::jmax(mLo, mHi));
        aModCur = toAngle(mCur);

        // 帯域ゲインのように「段数で割った結果、1 段あたりの振れ幅が極小」に
        // なる行き先では、比例のまま描くと 1px 未満になって何も見えない。
        // 変調が掛かっていること自体は必ず分かるよう、最小 10px 相当を確保する。
        const float minSpan = 10.0f / juce::jmax(8.0f, r);
        if (aHi - aLo < minSpan) {
            const float mid = (aLo + aHi) * 0.5f;
            aLo = mid - minSpan * 0.5f;
            aHi = mid + minSpan * 0.5f;
            // 端にいるときにトラック外へはみ出さないよう押し戻す
            if (aLo < startAngle) { aHi += startAngle - aLo; aLo = startAngle; }
            if (aHi > endAngle)   { aLo -= aHi - endAngle;   aHi = endAngle;   }
            aLo = juce::jmax(aLo, startAngle);

            // 帯を広げた分、マーカーも「表示上の帯の中での位置」に読み替える。
            // 真の角度のままだと 1px 未満しか動かず、止まって見えてしまう。
            const double t = (mHi > mLo) ? juce::jlimit(0.0, 1.0, (mCur - mLo) / (mHi - mLo)) : 0.5;
            aModCur = aLo + static_cast<float>(t) * (aHi - aLo);
        }

        juce::Path span; span.addCentredArc(cx, cy, r, r, 0.0f, aLo, aHi, true);
        g.setColour(MOColors::mint.withAlpha(0.62f));
        g.strokePath(span, juce::PathStrokeType(th,
            juce::PathStrokeType::curved, juce::PathStrokeType::butt));
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

    // --- MOD の現在位置マーカー ---
    // 【重要】必ずハンドルより後に描くこと。
    // 帯域ゲインのように変調量が小さい行き先では、マーカーがハンドルと
    // ほぼ同じ位置に来る。先に描くと白いハンドル (約 4.4px) に完全に隠れ、
    // 「変調が効いていない」ように見えてしまう。
    // リングより少しはみ出させ、背景色の縁取りでコントラストも確保する。
    if (hasMod) {
        const float sn = std::sin(aModCur), cs = std::cos(aModCur);
        const float rIn  = r - th * 0.78f;
        const float rOut = r + th * 0.78f;
        const float x1 = cx + sn * rIn,  y1 = cy - cs * rIn;
        const float x2 = cx + sn * rOut, y2 = cy - cs * rOut;

        g.setColour(MOColors::bg);
        g.drawLine(x1, y1, x2, y2, 5.0f);
        g.setColour(MOColors::mint.brighter(0.7f));
        g.drawLine(x1, y1, x2, y2, 2.6f);
    }

    // 中心に数値。桁数が多いときだけ字を詰めて、短い値はできるだけ大きく出す。
    const juce::String txt = formatValue(slider.getValue());
    float fontH = juce::jlimit(10.0f, 18.0f, capR * 0.86f);
    if (txt.length() >= 5)      fontH = juce::jmin(fontH, capR * 0.62f);
    else if (txt.length() == 4) fontH = juce::jmin(fontH, capR * 0.72f);

    g.setColour(slider.isEnabled() ? MOColors::text : MOColors::textDim);
    g.setFont(juce::Font(juce::FontOptions(fontH, juce::Font::bold)));
    g.drawText(txt,
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

    // --- MOD レンジ (バーの上下いっぱいに薄い帯 + 現在位置の縦線) ---
    double mLo, mHi, mCur;
    if (getModSpan(slider, mLo, mHi, mCur)) {
        auto toX = [&](double v) {
            return b.getX() + b.getWidth() * (float)slider.valueToProportionOfLength(v);
            };
        float xa = toX(juce::jmin(mLo, mHi));
        float xb = toX(juce::jmax(mLo, mHi));

        // ノブ側と同じ理由で最小表示幅を確保する
        if (xb - xa < 6.0f) {
            const float mid = (xa + xb) * 0.5f;
            xa = juce::jmax(b.getX(), mid - 3.0f);
            xb = juce::jmin(b.getRight(), xa + 6.0f);
        }
        g.setColour(MOColors::mint.withAlpha(0.34f));
        g.fillRoundedRectangle(xa, b.getY(), juce::jmax(1.0f, xb - xa), b.getHeight(), 3.0f);
        const float xc = toX(mCur);
        g.setColour(MOColors::mint);
        g.fillRect(xc - 1.0f, b.getY() + 1.0f, 2.0f, b.getHeight() - 2.0f);
    }

    // 文字は「塗り部分」と「未塗り部分」でクリップを分けて 2 回描く。
    // 明るいバーの上に明るい文字が乗って消える問題を根本から回避する。
    const auto textArea = b.reduced(7.0f, 0.0f);
    const juce::String nameTxt = slider.getName();
    const juce::String valTxt  = formatValue(slider.getValue());

    auto drawPair = [&](juce::Colour nameCol, juce::Colour valCol) {
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
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
    return juce::Font(juce::FontOptions(12.0f, juce::Font::bold));
}

juce::Font MultiOtoLookAndFeel::getComboBoxFont(juce::ComboBox&) {
    return juce::Font(juce::FontOptions(13.5f, juce::Font::bold));
}

juce::Font MultiOtoLookAndFeel::getPopupMenuFont() {
    return juce::Font(juce::FontOptions(14.0f, juce::Font::plain));
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
        label.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
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
        auto lab = r.removeFromBottom(15);
        label.setBounds(lab.expanded(6, 0));
    }
    slider.setBounds(r);
}

void ArcKnob::setVisible(bool v) {
    slider.setVisible(v);
    if (hasLabel) label.setVisible(v);
}

void ArcKnob::setAccent(juce::Colour c) {
    slider.setColour(juce::Slider::rotarySliderFillColourId, c);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, MOColors::knobTrack);
    if (hasLabel) label.setColour(juce::Label::textColourId, MOColors::textDim);
    slider.repaint();
}
