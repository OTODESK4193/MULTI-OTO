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
int DynVisualComponent::getBandAtPosition (juce::Point<int> pos) const
{
    auto area = getLocalBounds().reduced (10, 8);
    area.removeFromTop (22);   // タイトルヘッダー

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

// --- マウスホバー ＆ カーソル変更 ---
void DynVisualComponent::mouseMove (const juce::MouseEvent& e)
{
    int prevHover = hoveredBand;
    hoveredBand = getBandAtPosition (e.getPosition());

    if (hoveredBand != -1)
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

// --- ドラッグ操作 ---
void DynVisualComponent::mouseDown (const juce::MouseEvent& e)
{
    draggedBand = getBandAtPosition (e.getPosition());
    if (draggedBand == -1) return;

    // Shiftキー押下、またはエリアの上半分/下半分でターゲット判定
    auto area = getLocalBounds().reduced (10, 8);
    area.removeFromTop (22);
    float midY = area.getY() + area.getHeight() * 0.5f;

    if (e.mods.isShiftDown())
    {
        dragTarget = (e.y < midY) ? TargetUpward : TargetDownward;
    }
    else
    {
        dragTarget = TargetGain;
    }

    // 初期値の記憶
    if (dragTarget == TargetGain && paramGain[draggedBand] != nullptr)
    {
        dragStartValue = paramGain[draggedBand]->getValue();
    }
    else if (dragTarget == TargetUpward && paramUp[draggedBand] != nullptr)
    {
        dragStartValue = paramUp[draggedBand]->getValue();
    }
    else if (dragTarget == TargetDownward && paramDown[draggedBand] != nullptr)
    {
        dragStartValue = paramDown[draggedBand]->getValue();
    }
}

void DynVisualComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (draggedBand == -1) return;

    // マウスの上下移動距離 (Y軸は下が正なので反転)
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
        // 0.0 dB (正規化値 0.5) にリセット
        paramGain[band]->setValueNotifyingHost (0.5f);
        repaint();
    }
}

// ============================================================================
void DynVisualComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    MOColors::paintWell (g, bounds);

    auto area = bounds.reduced (10, 8);

    // --- タイトル ＆ クロスオーバー周波数 ---
    auto headerRow = area.removeFromTop (18);
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.setColour (MOColors::babyBlue);
    g.drawText (title, headerRow, juce::Justification::centredLeft);

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

    for (int b = 0; b < 3; ++b)
    {
        int bx = area.getX() + b * (bandW + 6);
        auto bandRect = juce::Rectangle<int> (bx, area.getY(), bandW, bandH);

        // ホバー時の枠強調
        if (b == hoveredBand || b == draggedBand)
        {
            g.setColour (MOColors::accent.withAlpha (0.12f));
            g.fillRoundedRectangle (bandRect.toFloat(), 4.0f);
            g.setColour (MOColors::accent.withAlpha (0.40f));
            g.drawRoundedRectangle (bandRect.toFloat().reduced (0.5f), 4.0f, 1.5f);
        }
        else
        {
            g.setColour (MOColors::panelLine.withAlpha (0.06f));
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

        // 中央基準線 (0 dB)
        g.setColour (MOColors::grid.withAlpha (0.20f));
        g.drawHorizontalLine ((int) midY, (float) bx + 2, (float) (bx + bandW - 2));

        // ゲイン変化バー
        float gainNorm = gainToNorm (smoothGainDb[b]);

        if (gainNorm > 0.01f)
        {
            // Upward (基準線より上)
            float barH = gainNorm * (bandH * 0.45f);
            auto upRect = juce::Rectangle<float> (
                (float) bx + 4, midY - barH, (float) bandW - 8, barH);
            g.setColour (MOColors::mint.withAlpha (0.35f));
            g.fillRoundedRectangle (upRect, 2.0f);
            g.setColour (MOColors::mint);
            g.fillRect (upRect.getX(), upRect.getY(), upRect.getWidth(), 2.0f);
        }
        else if (gainNorm < -0.01f)
        {
            // Downward (基準線より下)
            float barH = -gainNorm * (bandH * 0.45f);
            auto dnRect = juce::Rectangle<float> (
                (float) bx + 4, midY, (float) bandW - 8, barH);
            g.setColour (MOColors::pink.withAlpha (0.35f));
            g.fillRoundedRectangle (dnRect, 2.0f);
            g.setColour (MOColors::pink);
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

        g.setColour (smoothGainDb[b] > 0.05f ? MOColors::mint : (smoothGainDb[b] < -0.05f ? MOColors::pink : MOColors::textDim));
        g.drawText (dbStr, bx, area.getY() + 4, bandW, 16, juce::Justification::centred);

        // バンド名
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.setColour (MOColors::textDim);
        g.drawText (bandNames[b], bx, area.getY() + bandH - 16, bandW, 16, juce::Justification::centred);
    }
}
