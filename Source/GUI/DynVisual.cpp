#include "DynVisual.h"
#include "DSP/EngineCore.h"

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
    int bandW  = (totalW - 12) / 3;

    for (int b = 0; b < 3; ++b)
    {
        int bx = area.getX() + b * (bandW + 6);
        auto bandRect = juce::Rectangle<int> (bx, area.getY(), bandW, area.getHeight());
        if (bandRect.contains (pos))
            return b;
    }
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

    // クリック位置・修飾キーによる目的パラメータの明確な分離
    if (e.mods.isShiftDown() || e.mods.isAltDown())
    {
        dragTarget = TargetGain;
    }
    else if ((float) e.y < midY)
    {
        dragTarget = TargetUpward;    // 上半分 ➔ UPWARD のみ変更
    }
    else
    {
        dragTarget = TargetDownward;  // 下半分 ➔ DOWNWARD のみ変更
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
        paramGain[band]->setValueNotifyingHost (0.5f); // 0.0 dB リセット
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

    if (xoverLo != nullptr && xoverHi != nullptr)
    {
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.setColour (MOColors::textDim);
        auto loF = xoverLo->load (std::memory_order_relaxed);
        auto hiF = xoverHi->load (std::memory_order_relaxed);
        g.drawText (freqToString (loF) + " / " + freqToString (hiF),
                    headerRow, juce::Justification::centredRight);
    }

    area.removeFromTop (4);

    const char* bandNames[3] = { "LOW", "MID", "HIGH" };
    int totalW = area.getWidth();
    int bandW  = (totalW - 12) / 3;
    int bandH  = area.getHeight();
    float midY = area.getY() + bandH * 0.5f;

    const juce::Colour upColors[3] = { MOColors::bandLowUp, MOColors::bandMidUp, MOColors::bandHighUp };
    const juce::Colour dnColors[3] = { MOColors::bandLowDn, MOColors::bandMidDn, MOColors::bandHighDn };

    for (int b = 0; b < 3; ++b)
    {
        int bx = area.getX() + b * (bandW + 6);
        auto bandRect = juce::Rectangle<int> (bx, area.getY(), bandW, bandH);

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
            auto envRect = juce::Rectangle<int> (bx + 2, area.getY() + bandH - envH, bandW - 4, envH);
            g.setColour (MOColors::text.withAlpha (0.08f));
            g.fillRoundedRectangle (envRect.toFloat(), 3.0f);
        }

        // Upward / Downward 設定値シェードガイド
        float upPct   = (paramUp[b] != nullptr)   ? paramUp[b]->getValue()   : 1.0f;
        float downPct = (paramDown[b] != nullptr) ? paramDown[b]->getValue() : 1.0f;

        float maxUpH = (bandH * 0.42f) * upPct;
        if (maxUpH > 1.0f)
        {
            auto upGuide = juce::Rectangle<float> ((float) bx + 2, midY - maxUpH, (float) bandW - 4, maxUpH);
            g.setColour (upColors[b].withAlpha (0.08f));
            g.fillRoundedRectangle (upGuide, 2.0f);
            g.setColour (upColors[b].withAlpha (0.25f));
            g.drawHorizontalLine ((int) (midY - maxUpH), (float) bx + 3, (float) (bx + bandW - 3));
        }

        float maxDnH = (bandH * 0.42f) * downPct;
        if (maxDnH > 1.0f)
        {
            auto dnGuide = juce::Rectangle<float> ((float) bx + 2, midY, (float) bandW - 4, maxDnH);
            g.setColour (dnColors[b].withAlpha (0.08f));
            g.fillRoundedRectangle (dnGuide, 2.0f);
            g.setColour (dnColors[b].withAlpha (0.25f));
            g.drawHorizontalLine ((int) (midY + maxDnH), (float) bx + 3, (float) (bx + bandW - 3));
        }

        // 中央基準線 (0 dB)
        g.setColour (MOColors::grid.withAlpha (0.20f));
        g.drawHorizontalLine ((int) midY, (float) bx + 2, (float) (bx + bandW - 2));

        // ゲイン変化バー
        float gainNorm = gainToNorm (smoothGainDb[b]);

        if (gainNorm > 0.01f)
        {
            float barH = gainNorm * (bandH * 0.42f);
            auto upRect = juce::Rectangle<float> (
                (float) bx + 4, midY - barH, (float) bandW - 8, barH);
            g.setColour (upColors[b].withAlpha (0.45f));
            g.fillRoundedRectangle (upRect, 2.0f);
            g.setColour (upColors[b]);
            g.fillRect (upRect.getX(), upRect.getY(), upRect.getWidth(), 2.0f);
        }
        else if (gainNorm < -0.01f)
        {
            float barH = -gainNorm * (bandH * 0.42f);
            auto dnRect = juce::Rectangle<float> (
                (float) bx + 4, midY, (float) bandW - 8, barH);
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
        g.drawText (dbStr, bx, area.getY() + 3, bandW, 16, juce::Justification::centred);

        // バンド名
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.setColour (upColors[b].withAlpha (0.9f));
        g.drawText (bandNames[b], bx, area.getY() + bandH - 16, bandW, 16, juce::Justification::centred);
    }
}
