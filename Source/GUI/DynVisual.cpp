#include "DynVisual.h"
#include "DSP/EngineCore.h"

// 対数周波数スケール変換 (20Hz ~ 20000Hz ➔ 0.0 ~ 1.0)
static float logFreqToNorm (float f)
{
    static const float minLog = std::log10 (20.0f);
    static const float maxLog = std::log10 (20000.0f);
    float clampF = juce::jlimit (20.0f, 20000.0f, f);
    return (std::log10 (clampF) - minLog) / (maxLog - minLog);
}

// ============================================================================
void DynVisualComponent::timerCallback()
{
    if (meter == nullptr) return;

    for (int i = 0; i < 3; ++i)
    {
        float env  = meter->envDb[i].load (std::memory_order_relaxed);
        float gain = meter->gainDb[i].load (std::memory_order_relaxed);

        smoothEnvDb[i]  += kAlpha * (env  - smoothEnvDb[i]);
        smoothGainDb[i] += kAlpha * (gain - smoothGainDb[i]);
    }
    repaint();
}

// ============================================================================
bool DynVisualComponent::isHeaderPosition (juce::Point<int> pos) const
{
    return pos.y <= 24;
}

int DynVisualComponent::getBandAtPosition (juce::Point<int> pos) const
{
    if (isHeaderPosition (pos)) return -1;

    auto area = getLocalBounds().reduced (8, 6);
    area.removeFromTop (22);

    int totalW = area.getWidth();
    float loF = (xoverLo != nullptr) ? xoverLo->load (std::memory_order_relaxed) : 88.0f;
    float hiF = (xoverHi != nullptr) ? xoverHi->load (std::memory_order_relaxed) : 2500.0f;

    float normLo = logFreqToNorm (loF);
    float normHi = logFreqToNorm (hiF);

    // 最小幅マージン (各 35px) を保証した動的幅計算
    int gap = 4;
    int availW = totalW - gap * 2;
    int minW = 35;

    int wLow  = juce::jlimit (minW, availW - minW * 2, (int) (availW * normLo));
    int wMid  = juce::jlimit (minW, availW - wLow - minW, (int) (availW * (normHi - normLo)));
    int wHigh = std::max (minW, availW - wLow - wMid);

    int xLow  = area.getX();
    int xMid  = xLow + wLow + gap;
    int xHigh = xMid + wMid + gap;

    if (pos.x >= xLow && pos.x < xLow + wLow) return 0;
    if (pos.x >= xMid && pos.x < xMid + wMid) return 1;
    if (pos.x >= xHigh && pos.x < xHigh + wHigh) return 2;

    return -1;
}

void DynVisualComponent::mouseMove (const juce::MouseEvent& e)
{
    int prevHover = hoveredBand;
    hoveredBand = getBandAtPosition (e.getPosition());

    if (isHeaderPosition (e.getPosition()))
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    else if (hoveredBand != -1)
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    else
        setMouseCursor (juce::MouseCursor::NormalCursor);

    if (prevHover != hoveredBand)
        repaint();
}

void DynVisualComponent::mouseExit (const juce::MouseEvent&)
{
    if (hoveredBand != -1)
    {
        hoveredBand = -1;
        setMouseCursor (juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void DynVisualComponent::mouseDown (const juce::MouseEvent& e)
{
    if (isHeaderPosition (e.getPosition()))
    {
        if (onStageSelected) onStageSelected (stage);
        return;
    }

    draggedBand = getBandAtPosition (e.getPosition());
    if (draggedBand == -1) return;

    auto area = getLocalBounds().reduced (8, 6);
    area.removeFromTop (22);
    float midY = area.getY() + area.getHeight() * 0.5f;

    if (e.mods.isShiftDown() || e.mods.isAltDown())
    {
        dragTarget = TargetGain;
    }
    else if ((float) e.y < midY)
    {
        dragTarget = TargetUpward;    // 上半分 ➔ UPWARD
    }
    else
    {
        dragTarget = TargetDownward;  // 下半分 ➔ DOWNWARD
    }

    if (dragTarget == TargetGain && paramGain[draggedBand] != nullptr)
        dragStartValue = paramGain[draggedBand]->getValue();
    else if (dragTarget == TargetUpward && paramUp[draggedBand] != nullptr)
        dragStartValue = paramUp[draggedBand]->getValue();
    else if (dragTarget == TargetDownward && paramDown[draggedBand] != nullptr)
        dragStartValue = paramDown[draggedBand]->getValue();
}

void DynVisualComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (draggedBand == -1) return;

    float deltaY = (float) -e.getDistanceFromDragStartY();
    float sensitivity = e.mods.isAltDown() ? 0.0005f : 0.003f;

    if (dragTarget == TargetGain && paramGain[draggedBand] != nullptr)
    {
        float newVal = juce::jlimit (0.0f, 1.0f, dragStartValue + deltaY * sensitivity);
        paramGain[draggedBand]->setValueNotifyingHost (newVal);
    }
    else if (dragTarget == TargetUpward && paramUp[draggedBand] != nullptr)
    {
        float newVal = juce::jlimit (0.0f, 1.0f, dragStartValue + deltaY * sensitivity);
        paramUp[draggedBand]->setValueNotifyingHost (newVal);
    }
    else if (dragTarget == TargetDownward && paramDown[draggedBand] != nullptr)
    {
        float newVal = juce::jlimit (0.0f, 1.0f, dragStartValue + deltaY * sensitivity);
        paramDown[draggedBand]->setValueNotifyingHost (newVal);
    }
    repaint();
}

void DynVisualComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    int band = getBandAtPosition (e.getPosition());
    if (band != -1 && paramGain[band] != nullptr)
    {
        paramGain[band]->setValueNotifyingHost (0.5f); // 0.0 dB
        repaint();
    }
}

// ============================================================================
void DynVisualComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    MOColors::paintWell (g, bounds);

    if (isSelected)
    {
        g.setColour (stage == 1 ? MOColors::peach : MOColors::babyBlue);
        g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), 4.0f, 2.0f);
    }

    auto area = bounds.reduced (8, 6);

    // --- タイトル部 ---
    auto headerRow = area.removeFromTop (22);
    auto titleBtnArea = headerRow.removeFromLeft (130);

    g.setColour (isSelected ? (stage == 1 ? MOColors::peach.withAlpha (0.25f) : MOColors::babyBlue.withAlpha (0.25f))
                            : MOColors::knobTrack);
    g.fillRoundedRectangle (titleBtnArea.toFloat(), 3.0f);

    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.setColour (isSelected ? (stage == 1 ? MOColors::peach : MOColors::babyBlue) : MOColors::textDim);
    g.drawText (title, titleBtnArea, juce::Justification::centred);

    float loF = 88.0f, hiF = 2500.0f;
    if (xoverLo != nullptr && xoverHi != nullptr)
    {
        loF = xoverLo->load (std::memory_order_relaxed);
        hiF = xoverHi->load (std::memory_order_relaxed);
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.setColour (MOColors::textDim);
        g.drawText (freqToString (loF) + " / " + freqToString (hiF),
                    headerRow, juce::Justification::centredRight);
    }

    area.removeFromTop (4);

    const char* bandNames[3] = { "LOW", "MID", "HIGH" };
    int totalW = area.getWidth();
    int bandH  = area.getHeight();
    float midY = area.getY() + bandH * 0.5f;

    // --- クロスオーバー周波数連動の対数幅計算 ---
    float normLo = logFreqToNorm (loF);
    float normHi = logFreqToNorm (hiF);

    int gap = 4;
    int availW = totalW - gap * 2;
    int minW = 35;

    int bandW[3];
    bandW[0] = juce::jlimit (minW, availW - minW * 2, (int) (availW * normLo));
    bandW[1] = juce::jlimit (minW, availW - bandW[0] - minW, (int) (availW * (normHi - normLo)));
    bandW[2] = std::max (minW, availW - bandW[0] - bandW[1]);

    int bandX[3];
    bandX[0] = area.getX();
    bandX[1] = bandX[0] + bandW[0] + gap;
    bandX[2] = bandX[1] + bandW[1] + gap;

    const juce::Colour upColors[3] = { MOColors::bandLowUp, MOColors::bandMidUp, MOColors::bandHighUp };
    const juce::Colour dnColors[3] = { MOColors::bandLowDn, MOColors::bandMidDn, MOColors::bandHighDn };

    for (int b = 0; b < 3; ++b)
    {
        int bx = bandX[b];
        int bw = bandW[b];
        auto bandRect = juce::Rectangle<int> (bx, area.getY(), bw, bandH);

        if (b == hoveredBand || b == draggedBand)
        {
            g.setColour (upColors[b].withAlpha (0.12f));
            g.fillRoundedRectangle (bandRect.toFloat(), 4.0f);
            g.setColour (upColors[b].withAlpha (0.50f));
            g.drawRoundedRectangle (bandRect.toFloat().reduced (0.5f), 4.0f, 1.5f);
        }
        else
        {
            g.setColour (MOColors::panelLine.withAlpha (0.05f));
            g.fillRoundedRectangle (bandRect.toFloat(), 4.0f);
        }

        // 入力レベル背景バー
        float envNorm = levelToNorm (smoothEnvDb[b]);
        if (envNorm > 0.001f)
        {
            int envH = (int) (bandH * envNorm);
            auto envRect = juce::Rectangle<int> (bx + 2, area.getY() + bandH - envH, bw - 4, envH);
            g.setColour (MOColors::text.withAlpha (0.08f));
            g.fillRoundedRectangle (envRect.toFloat(), 3.0f);
        }

        // Upward / Downward 設定値シェードガイド
        float upPct   = (paramUp[b] != nullptr)   ? paramUp[b]->getValue()   : 1.0f;
        float downPct = (paramDown[b] != nullptr) ? paramDown[b]->getValue() : 1.0f;

        float maxUpH = (bandH * 0.42f) * upPct;
        if (maxUpH > 1.0f)
        {
            auto upGuide = juce::Rectangle<float> ((float) bx + 2, midY - maxUpH, (float) bw - 4, maxUpH);
            g.setColour (upColors[b].withAlpha (0.08f));
            g.fillRoundedRectangle (upGuide, 2.0f);
            g.setColour (upColors[b].withAlpha (0.25f));
            g.drawHorizontalLine ((int) (midY - maxUpH), (float) bx + 3, (float) (bx + bw - 3));
        }

        float maxDnH = (bandH * 0.42f) * downPct;
        if (maxDnH > 1.0f)
        {
            auto dnGuide = juce::Rectangle<float> ((float) bx + 2, midY, (float) bw - 4, maxDnH);
            g.setColour (dnColors[b].withAlpha (0.08f));
            g.fillRoundedRectangle (dnGuide, 2.0f);
            g.setColour (dnColors[b].withAlpha (0.25f));
            g.drawHorizontalLine ((int) (midY + maxDnH), (float) bx + 3, (float) (bx + bw - 3));
        }

        // 中央基準線 (0 dB)
        g.setColour (MOColors::grid.withAlpha (0.20f));
        g.drawHorizontalLine ((int) midY, (float) bx + 2, (float) (bx + bw - 2));

        // ゲイン変化バー
        float gainNorm = gainToNorm (smoothGainDb[b]);

        if (gainNorm > 0.01f)
        {
            float barH = gainNorm * (bandH * 0.42f);
            auto upRect = juce::Rectangle<float> (
                (float) bx + 4, midY - barH, (float) bw - 8, barH);
            g.setColour (upColors[b].withAlpha (0.45f));
            g.fillRoundedRectangle (upRect, 2.0f);
            g.setColour (upColors[b]);
            g.fillRect (upRect.getX(), upRect.getY(), upRect.getWidth(), 2.0f);
        }
        else if (gainNorm < -0.01f)
        {
            float barH = -gainNorm * (bandH * 0.42f);
            auto dnRect = juce::Rectangle<float> (
                (float) bx + 4, midY, (float) bw - 8, barH);
            g.setColour (dnColors[b].withAlpha (0.45f));
            g.fillRoundedRectangle (dnRect.toFloat(), 2.0f);
            g.setColour (dnColors[b]);
            g.fillRect (dnRect.getX(), dnRect.getBottom() - 2.0f, dnRect.getWidth(), 2.0f);
        }

        // ゲイン数値 (dB)
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        juce::String dbStr;
        if (smoothGainDb[b] > 0.05f)
            dbStr = "+" + juce::String (smoothGainDb[b], 1) + "dB";
        else if (smoothGainDb[b] < -0.05f)
            dbStr = juce::String (smoothGainDb[b], 1) + "dB";
        else
            dbStr = "0.0dB";

        g.setColour (smoothGainDb[b] > 0.05f ? upColors[b] : (smoothGainDb[b] < -0.05f ? dnColors[b] : MOColors::textDim));
        g.drawText (dbStr, bx, area.getY() + 3, bw, 16, juce::Justification::centred);

        // バンド名 (幅に応じて適切に表示)
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.setColour (upColors[b].withAlpha (0.9f));
        g.drawText (bandNames[b], bx, area.getY() + bandH - 16, bw, 16, juce::Justification::centred);
    }
}
