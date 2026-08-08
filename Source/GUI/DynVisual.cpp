#include "DynVisual.h"
#include "DSP/EngineCore.h"

// 対数周波数スケール変換 (20Hz ~ 20000Hz ➔ 0.0 ~ 1.0)
static const float kMinLog = std::log10 (20.0f);
static const float kMaxLog = std::log10 (20000.0f);

static float logFreqToNorm (float f)
{
    float clampF = juce::jlimit (20.0f, 20000.0f, f);
    return (std::log10 (clampF) - kMinLog) / (kMaxLog - kMinLog);
}

static float normToLogFreq (float n)
{
    return std::pow (10.0f, kMinLog + juce::jlimit (0.0f, 1.0f, n) * (kMaxLog - kMinLog));
}

// ============================================================================
float DynVisualComponent::getLoFreq() const
{
    if (paramXLow == nullptr) return 88.0f;
    return paramXLow->convertFrom0to1 (paramXLow->getValue());
}

float DynVisualComponent::getHiFreq() const
{
    if (paramXHigh == nullptr) return 2500.0f;
    return paramXHigh->convertFrom0to1 (paramXHigh->getValue());
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

void DynVisualComponent::computeBandLayout (juce::Rectangle<int>& areaOut, int bandX[3], int bandW[3]) const
{
    auto area = getLocalBounds().reduced (8, 6);
    area.removeFromTop (22);
    area.removeFromTop (4);
    areaOut = area;

    const float normLo = logFreqToNorm (getLoFreq());
    const float normHi = logFreqToNorm (getHiFreq());

    const int gap   = 4;
    const int availW = area.getWidth() - gap * 2;
    const int minW   = 35;

    bandW[0] = juce::jlimit (minW, availW - minW * 2, (int) (availW * normLo));
    bandW[1] = juce::jlimit (minW, availW - bandW[0] - minW, (int) (availW * (normHi - normLo)));
    bandW[2] = std::max (minW, availW - bandW[0] - bandW[1]);

    bandX[0] = area.getX();
    bandX[1] = bandX[0] + bandW[0] + gap;
    bandX[2] = bandX[1] + bandW[1] + gap;
}

int DynVisualComponent::getBandAtPosition (juce::Point<int> pos) const
{
    if (isHeaderPosition (pos)) return -1;

    juce::Rectangle<int> area; int bx[3], bw[3];
    computeBandLayout (area, bx, bw);

    for (int b = 0; b < 3; ++b)
        if (pos.x >= bx[b] && pos.x < bx[b] + bw[b]) return b;

    return -1;
}

int DynVisualComponent::getBoundaryAtPosition (juce::Point<int> pos) const
{
    if (isHeaderPosition (pos)) return -1;
    if (paramXLow == nullptr || paramXHigh == nullptr) return -1;

    juce::Rectangle<int> area; int bx[3], bw[3];
    computeBandLayout (area, bx, bw);

    if (pos.y < area.getY() || pos.y > area.getBottom()) return -1;

    // 境界は「前のバンドの右端」と「次のバンドの左端」の中間
    const int boundary0 = (bx[0] + bw[0] + bx[1]) / 2;
    const int boundary1 = (bx[1] + bw[1] + bx[2]) / 2;

    if (std::abs (pos.x - boundary0) <= kBoundaryGrab) return 0;
    if (std::abs (pos.x - boundary1) <= kBoundaryGrab) return 1;
    return -1;
}

void DynVisualComponent::applyBoundaryDrag (int boundary, int mouseX)
{
    juce::Rectangle<int> area; int bx[3], bw[3];
    computeBandLayout (area, bx, bw);

    const int gap    = 4;
    const int availW = area.getWidth() - gap * 2;
    if (availW <= 0) return;

    const float norm = (float) (mouseX - area.getX()) / (float) availW;
    const float freq = normToLogFreq (norm);

    auto* p = (boundary == 0) ? paramXLow : paramXHigh;
    if (p == nullptr) return;

    const auto& range = p->getNormalisableRange();
    const float clamped = juce::jlimit (range.start, range.end, freq);

    p->setValueNotifyingHost (p->convertTo0to1 (clamped));
    repaint();
}

// ============================================================================
void DynVisualComponent::mouseMove (const juce::MouseEvent& e)
{
    const int prevHover    = hoveredBand;
    const int prevBoundary = hoveredBoundary;

    hoveredBoundary = getBoundaryAtPosition (e.getPosition());
    hoveredBand     = (hoveredBoundary >= 0) ? -1 : getBandAtPosition (e.getPosition());

    if (hoveredBoundary >= 0)
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    else if (isHeaderPosition (e.getPosition()) && onStageSelected != nullptr)
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    else if (hoveredBand != -1)
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    else
        setMouseCursor (juce::MouseCursor::NormalCursor);

    if (prevHover != hoveredBand || prevBoundary != hoveredBoundary)
        repaint();
}

void DynVisualComponent::mouseExit (const juce::MouseEvent&)
{
    if (hoveredBand != -1 || hoveredBoundary != -1)
    {
        hoveredBand = -1;
        hoveredBoundary = -1;
        setMouseCursor (juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void DynVisualComponent::mouseDown (const juce::MouseEvent& e)
{
    draggedBand = -1;
    draggedBoundary = -1;

    // 境界ドラッグを最優先 (バンドドラッグより手前で判定する)
    const int boundary = getBoundaryAtPosition (e.getPosition());
    if (boundary >= 0)
    {
        draggedBoundary = boundary;
        if (auto* p = (boundary == 0) ? paramXLow : paramXHigh)
            p->beginChangeGesture();
        return;
    }

    if (isHeaderPosition (e.getPosition()))
    {
        if (onStageSelected != nullptr) onStageSelected (stage);
        return;
    }

    draggedBand = getBandAtPosition (e.getPosition());
    if (draggedBand == -1) return;

    juce::Rectangle<int> area; int bx[3], bw[3];
    computeBandLayout (area, bx, bw);
    const float midY = area.getY() + area.getHeight() * 0.5f;

    if (e.mods.isShiftDown() || e.mods.isAltDown())
        dragTarget = TargetGain;
    else if ((float) e.y < midY)
        dragTarget = TargetUpward;    // 上半分 ➔ UPWARD
    else
        dragTarget = TargetDownward;  // 下半分 ➔ DOWNWARD

    if (dragTarget == TargetGain && paramGain[draggedBand] != nullptr)
        dragStartValue = paramGain[draggedBand]->getValue();
    else if (dragTarget == TargetUpward && paramUp[draggedBand] != nullptr)
        dragStartValue = paramUp[draggedBand]->getValue();
    else if (dragTarget == TargetDownward && paramDown[draggedBand] != nullptr)
        dragStartValue = paramDown[draggedBand]->getValue();
}

void DynVisualComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (draggedBoundary >= 0)
    {
        applyBoundaryDrag (draggedBoundary, e.x);
        return;
    }

    if (draggedBand == -1) return;

    const float deltaY = (float) -e.getDistanceFromDragStartY();
    const float sensitivity = e.mods.isAltDown() ? 0.0005f : 0.003f;
    const float newVal = juce::jlimit (0.0f, 1.0f, dragStartValue + deltaY * sensitivity);

    if (dragTarget == TargetGain && paramGain[draggedBand] != nullptr)
        paramGain[draggedBand]->setValueNotifyingHost (newVal);
    else if (dragTarget == TargetUpward && paramUp[draggedBand] != nullptr)
        paramUp[draggedBand]->setValueNotifyingHost (newVal);
    else if (dragTarget == TargetDownward && paramDown[draggedBand] != nullptr)
        paramDown[draggedBand]->setValueNotifyingHost (newVal);

    repaint();
}

void DynVisualComponent::mouseUp (const juce::MouseEvent&)
{
    if (draggedBoundary >= 0)
        if (auto* p = (draggedBoundary == 0) ? paramXLow : paramXHigh)
            p->endChangeGesture();

    draggedBand = -1;
    draggedBoundary = -1;
    repaint();
}

void DynVisualComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (getBoundaryAtPosition (e.getPosition()) >= 0) return;

    const int band = getBandAtPosition (e.getPosition());
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

    auto header = bounds.reduced (8, 6);

    // --- タイトル部 ---
    auto headerRow = header.removeFromTop (22);
    auto titleBtnArea = headerRow.removeFromLeft (120);

    g.setColour (isSelected ? (stage == 1 ? MOColors::peach.withAlpha (0.25f) : MOColors::babyBlue.withAlpha (0.25f))
                            : MOColors::knobTrack);
    g.fillRoundedRectangle (titleBtnArea.toFloat(), 3.0f);

    g.setFont (juce::Font (juce::FontOptions (12.5f, juce::Font::bold)));
    g.setColour (isSelected ? (stage == 1 ? MOColors::peach : MOColors::babyBlue) : MOColors::textDim);
    g.drawText (title, titleBtnArea, juce::Justification::centred);

    const float loF = getLoFreq();
    const float hiF = getHiFreq();

    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    g.setColour ((hoveredBoundary >= 0 || draggedBoundary >= 0) ? MOColors::text : MOColors::textDim);
    g.drawText (freqToString (loF) + " / " + freqToString (hiF),
                headerRow, juce::Justification::centredRight);

    // --- バンド ---
    juce::Rectangle<int> area; int bandX[3], bandW[3];
    computeBandLayout (area, bandX, bandW);

    const char* bandNames[3] = { "LOW", "MID", "HIGH" };
    const int   bandH = area.getHeight();
    const float midY  = area.getY() + bandH * 0.5f;

    const juce::Colour upColors[3] = { MOColors::bandLowUp, MOColors::bandMidUp, MOColors::bandHighUp };
    const juce::Colour dnColors[3] = { MOColors::bandLowDn, MOColors::bandMidDn, MOColors::bandHighDn };

    for (int b = 0; b < 3; ++b)
    {
        const int bx = bandX[b];
        const int bw = bandW[b];
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
        const float envNorm = levelToNorm (smoothEnvDb[b]);
        if (envNorm > 0.001f)
        {
            const int envH = (int) (bandH * envNorm);
            auto envRect = juce::Rectangle<int> (bx + 2, area.getY() + bandH - envH, bw - 4, envH);
            g.setColour (MOColors::text.withAlpha (0.08f));
            g.fillRoundedRectangle (envRect.toFloat(), 3.0f);
        }

        // Upward / Downward 設定値シェードガイド
        const float upPct   = (paramUp[b]   != nullptr) ? paramUp[b]->getValue()   : 1.0f;
        const float downPct = (paramDown[b] != nullptr) ? paramDown[b]->getValue() : 1.0f;

        const float maxUpH = (bandH * 0.42f) * upPct;
        if (maxUpH > 1.0f)
        {
            auto upGuide = juce::Rectangle<float> ((float) bx + 2, midY - maxUpH, (float) bw - 4, maxUpH);
            g.setColour (upColors[b].withAlpha (0.08f));
            g.fillRoundedRectangle (upGuide, 2.0f);
            g.setColour (upColors[b].withAlpha (0.25f));
            g.drawHorizontalLine ((int) (midY - maxUpH), (float) bx + 3, (float) (bx + bw - 3));
        }

        const float maxDnH = (bandH * 0.42f) * downPct;
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

        // --- 帯域ゲインの MOD レンジ ---
        // メーターのバーは「実際に掛かっているゲイン」を出しているので、
        // そこに「どこまで振れるか」を薄い帯で重ねる。
        // ノブ側と同じく applyModToValue を通すので倍率は必ず一致する。
        if (matrix != nullptr && paramGain[b] != nullptr)
        {
            const int dst = (stage == 1 ? ModMatrix::DstS1GainL : ModMatrix::DstS2GainL) + b;
            const float rLo = matrix->getRangeMinForGui (dst);
            const float rHi = matrix->getRangeMaxForGui (dst);

            if (rHi - rLo > 1.0e-4f)
            {
                const double base = paramGain[b]->convertFrom0to1 (paramGain[b]->getValue());
                const double dLo  = matrix->applyModToValue (dst, base, rLo) - base;
                const double dHi  = matrix->applyModToValue (dst, base, rHi) - base;
                const double dCur = matrix->applyModToValue (dst, base, matrix->getForGui (dst)) - base;

                // 現在のバー位置から変調分を差し引いた「素の位置」を基準にする
                const float baseDb = smoothGainDb[b] - (float) dCur;
                const float y0 = midY - gainToNorm (baseDb + (float) dHi) * (bandH * 0.42f);
                const float y1 = midY - gainToNorm (baseDb + (float) dLo) * (bandH * 0.42f);

                float top = juce::jmin (y0, y1), bot = juce::jmax (y0, y1);
                if (bot - top < 4.0f) { const float m = (top + bot) * 0.5f; top = m - 2.0f; bot = m + 2.0f; }

                g.setColour (MOColors::mint.withAlpha (0.22f));
                g.fillRoundedRectangle ((float) bx + 4.0f, top, (float) bw - 8.0f, bot - top, 2.0f);
                g.setColour (MOColors::mint.withAlpha (0.55f));
                g.drawHorizontalLine ((int) top, (float) bx + 4.0f, (float) (bx + bw - 4));
                g.drawHorizontalLine ((int) bot, (float) bx + 4.0f, (float) (bx + bw - 4));
            }
        }

        // ゲイン変化バー
        const float gainNorm = gainToNorm (smoothGainDb[b]);

        if (gainNorm > 0.01f)
        {
            const float barH = gainNorm * (bandH * 0.42f);
            auto upRect = juce::Rectangle<float> ((float) bx + 4, midY - barH, (float) bw - 8, barH);
            g.setColour (upColors[b].withAlpha (0.45f));
            g.fillRoundedRectangle (upRect, 2.0f);
            g.setColour (upColors[b]);
            g.fillRect (upRect.getX(), upRect.getY(), upRect.getWidth(), 2.0f);
        }
        else if (gainNorm < -0.01f)
        {
            const float barH = -gainNorm * (bandH * 0.42f);
            auto dnRect = juce::Rectangle<float> ((float) bx + 4, midY, (float) bw - 8, barH);
            g.setColour (dnColors[b].withAlpha (0.45f));
            g.fillRoundedRectangle (dnRect, 2.0f);
            g.setColour (dnColors[b]);
            g.fillRect (dnRect.getX(), dnRect.getBottom() - 2.0f, dnRect.getWidth(), 2.0f);
        }

        // ゲイン数値 (dB)
        g.setFont (juce::Font (juce::FontOptions (12.5f, juce::Font::bold)));
        juce::String dbStr;
        if (smoothGainDb[b] > 0.05f)       dbStr = "+" + juce::String (smoothGainDb[b], 1) + "dB";
        else if (smoothGainDb[b] < -0.05f) dbStr = juce::String (smoothGainDb[b], 1) + "dB";
        else                               dbStr = "0.0dB";

        g.setColour (smoothGainDb[b] > 0.05f ? upColors[b]
                                             : (smoothGainDb[b] < -0.05f ? dnColors[b] : MOColors::textDim));
        g.drawText (dbStr, bx, area.getY() + 3, bw, 16, juce::Justification::centred);

        // バンド名
        g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
        g.setColour (upColors[b].withAlpha (0.9f));
        g.drawText (bandNames[b], bx, area.getY() + bandH - 16, bw, 16, juce::Justification::centred);
    }

    // --- 帯域境界ハンドル (ホバー/ドラッグ中に強調) ---
    for (int i = 0; i < 2; ++i)
    {
        const int hx = (i == 0) ? (bandX[0] + bandW[0] + bandX[1]) / 2
                                : (bandX[1] + bandW[1] + bandX[2]) / 2;
        const bool active = (hoveredBoundary == i || draggedBoundary == i);

        g.setColour (active ? MOColors::accent : MOColors::panelLine.withAlpha (0.18f));
        g.fillRoundedRectangle ((float) hx - (active ? 1.5f : 0.5f), (float) area.getY() + 2.0f,
                                active ? 3.0f : 1.0f, (float) bandH - 4.0f, 1.0f);

        if (active)
        {
            // ↔ を示すグリップ
            const float cy = area.getY() + bandH * 0.5f;
            g.setColour (MOColors::accent);
            for (int d = -1; d <= 1; d += 2)
            {
                juce::Path tri;
                tri.addTriangle ((float) hx + d * 4.0f, cy - 4.0f,
                                 (float) hx + d * 4.0f, cy + 4.0f,
                                 (float) hx + d * 9.0f, cy);
                g.fillPath (tri);
            }
        }
    }
}
