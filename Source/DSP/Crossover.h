#pragma once
#include <juce_dsp/juce_dsp.h>

class Crossover {
public:
    Crossover();
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    // 1サンプルのステレオ入力を受け取り、3バンド（Low, Mid, High）× ステレオに分割
    void process(float inL, float inR,
        float& lowL, float& lowR,
        float& midL, float& midR,
        float& highL, float& highR);

private:
    // Linkwitz-Riley 24dB/oct フィルター (ステレオ × 2分割ポイント = 計8基)
    juce::dsp::LinkwitzRileyFilter<float> lp1L, hp1L, lp2L, hp2L;
    juce::dsp::LinkwitzRileyFilter<float> lp1R, hp1R, lp2R, hp2R;
};