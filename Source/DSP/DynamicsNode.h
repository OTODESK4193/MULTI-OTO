#pragma once
#include <juce_dsp/juce_dsp.h>
#include <immintrin.h>
#include <cmath>
#include <algorithm>
#include "FastMath.h"

class DynamicsNode {
public:
    DynamicsNode() { reset(); }

    void prepare(double sr, int samplesPerBlock) {
        juce::ignoreUnused(samplesPerBlock);
        currentSampleRate = sr;
        reset();
    }

    void reset() { v_envSq = _mm256_setzero_ps(); }

    void setParameters(const float* gains, const float* depths, float macroTime, const float* attacks, const float* releases, float mix) {
        float timeScale = macroTime * 0.01f;
        alignas(32) float aArr[8] = { 1, 1, 1, 1, 1, 1, 1, 1 }, rArr[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
        alignas(32) float dArr[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }, gArr[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };

        for (int i = 0; i < 3; ++i) {
            float finalAtkMs = std::max(0.1f, attacks[i] * timeScale);
            float finalRelMs = std::max(1.0f, releases[i] * timeScale);
            float aC = 1.0f - std::exp(-1.0f / (finalAtkMs * 0.001f * static_cast<float>(currentSampleRate)));
            float rC = 1.0f - std::exp(-1.0f / (finalRelMs * 0.001f * static_cast<float>(currentSampleRate)));

            aArr[i * 2] = aC; aArr[i * 2 + 1] = aC;
            rArr[i * 2] = rC; rArr[i * 2 + 1] = rC;
            dArr[i * 2] = depths[i] * 0.01f; dArr[i * 2 + 1] = depths[i] * 0.01f;
            float gVal = std::pow(10.0f, gains[i] / 20.0f);
            gArr[i * 2] = gVal; gArr[i * 2 + 1] = gVal;
        }
        v_atk = _mm256_load_ps(aArr); v_rel = _mm256_load_ps(rArr);
        v_depth = _mm256_load_ps(dArr); v_gain = _mm256_load_ps(gArr);
        v_mix = _mm256_set1_ps(mix * 0.01f);
    }

    void process(juce::dsp::SIMDRegister<float>* simmData, int numSamples) {
        const __m256 db_scaler = _mm256_set1_ps(6.0205f), linear_scaler = _mm256_set1_ps(0.16609f);
        const __m256 down_thresh = _mm256_set1_ps(-15.0f), up_thresh = _mm256_set1_ps(-40.0f);
        const __m256 down_ratio_m = _mm256_set1_ps(-(1.0f - 0.25f)); // 1:4 Ratio
        const __m256 up_ratio_m = _mm256_set1_ps(0.5f);              // 1:2 Ratio
        const __m256 max_up_gain = _mm256_set1_ps(36.0f);            // 限界ゲイン制限

        __m256* buffer = reinterpret_cast<__m256*>(simmData);

        for (int i = 0; i < numSamples; ++i) {
            __m256 input = buffer[i];

            // RMSベースの滑らかなエンベロープ（リップル歪みを防ぐ）
            __m256 inSq = _mm256_mul_ps(input, input);
            __m256 isAttack = _mm256_cmp_ps(inSq, v_envSq, _CMP_GT_OQ);
            __m256 coeff = _mm256_blendv_ps(v_rel, v_atk, isAttack);

            v_envSq = _mm256_add_ps(v_envSq, _mm256_mul_ps(coeff, _mm256_sub_ps(inSq, v_envSq)));
            __m256 envRms = _mm256_sqrt_ps(_mm256_max_ps(v_envSq, _mm256_set1_ps(1e-15f)));
            __m256 envDb = _mm256_mul_ps(FastMath::fast_log2_256(envRms), db_scaler);

            // Downward & Upward (ゲートなし)
            __m256 overThresh = _mm256_max_ps(_mm256_setzero_ps(), _mm256_sub_ps(envDb, down_thresh));
            __m256 downGainDb = _mm256_mul_ps(_mm256_mul_ps(overThresh, down_ratio_m), v_depth);

            __m256 underThresh = _mm256_max_ps(_mm256_setzero_ps(), _mm256_sub_ps(up_thresh, envDb));
            __m256 rawUpGain = _mm256_mul_ps(underThresh, up_ratio_m);
            __m256 cappedUpGain = _mm256_min_ps(rawUpGain, max_up_gain);
            __m256 upGainDb = _mm256_mul_ps(cappedUpGain, v_depth);

            __m256 totalGainDb = _mm256_add_ps(downGainDb, upGainDb);
            __m256 linearGain = _mm256_mul_ps(FastMath::fast_exp2_256(_mm256_mul_ps(totalGainDb, linear_scaler)), v_gain);

            buffer[i] = _mm256_add_ps(input, _mm256_mul_ps(_mm256_sub_ps(_mm256_mul_ps(input, linearGain), input), v_mix));
        }
    }

private:
    double currentSampleRate = 44100.0;
    __m256 v_envSq, v_atk, v_rel, v_depth, v_gain, v_mix;
};