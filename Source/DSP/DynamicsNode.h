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

    void reset() {
        v_env = _mm256_setzero_ps();
    }

    void setParameters(const float* gains, const float* depths, float macroTime, const float* attacks, const float* releases) {
        float timeScale = macroTime * 0.01f;

        alignas(32) float aArr[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
        alignas(32) float rArr[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
        alignas(32) float dArr[8] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        alignas(32) float gArr[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

        // 0=Low, 1=Mid, 2=High
        for (int i = 0; i < 3; ++i) {
            float finalAtkMs = std::max(0.1f, attacks[i] * timeScale);
            float finalRelMs = std::max(1.0f, releases[i] * timeScale);

            float aC = 1.0f - std::exp(-1.0f / (finalAtkMs * 0.001f * static_cast<float>(currentSampleRate)));
            float rC = 1.0f - std::exp(-1.0f / (finalRelMs * 0.001f * static_cast<float>(currentSampleRate)));

            float dC = depths[i] * 0.01f;
            float gC = std::pow(10.0f, gains[i] / 20.0f);

            // Lch(2*i) と Rch(2*i+1) に配置
            aArr[i * 2] = aC; aArr[i * 2 + 1] = aC;
            rArr[i * 2] = rC; rArr[i * 2 + 1] = rC;
            dArr[i * 2] = dC; dArr[i * 2 + 1] = dC;
            gArr[i * 2] = gC; gArr[i * 2 + 1] = gC;
        }

        v_atk = _mm256_load_ps(aArr);
        v_rel = _mm256_load_ps(rArr);
        v_depth = _mm256_load_ps(dArr);
        v_gain = _mm256_load_ps(gArr);
    }

    /**
     * @param simmData EngineCoreから渡される SIMDRegister の配列ポインタ
     * @param numSamples バッファ長
     */
    void process(juce::dsp::SIMDRegister<float>* simmData, int numSamples) {
        // 定数のベクトル化
        const __m256 sign_mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));
        const __m256 db_scaler = _mm256_set1_ps(6.0205f);
        const __m256 linear_scaler = _mm256_set1_ps(0.16609f);

        const __m256 down_thresh = _mm256_set1_ps(downwardThresholdDb);
        const __m256 up_thresh = _mm256_set1_ps(upwardThresholdDb);
        const __m256 down_ratio_m = _mm256_set1_ps(-(1.0f - (1.0f / downwardRatio)));
        const __m256 up_ratio_m = _mm256_set1_ps(1.0f - upwardRatio);

        // Smart Safety Gate用設定: -90dB ～ -80dB の間でゲートを開閉
        const __m256 gate_floor = _mm256_set1_ps(-90.0f);
        const __m256 gate_scale = _mm256_set1_ps(0.1f); // 1.0 / 10.0dB
        const __m256 zero_ps = _mm256_setzero_ps();
        const __m256 one_ps = _mm256_set1_ps(1.0f);

        // EngineCore の配列を __m256 として直接走査
        __m256* buffer = reinterpret_cast<__m256*>(simmData);

        for (int i = 0; i < numSamples; ++i) {
            __m256 input = buffer[i];

            // 1. 絶対値とブランチレス・エンベロープ抽出
            __m256 inAbs = _mm256_and_ps(input, sign_mask);

            // (inAbs > v_env) なら v_atk、そうでなければ v_rel
            __m256 isAttackMask = _mm256_cmp_ps(inAbs, v_env, _CMP_GT_OQ);
            __m256 coeff = _mm256_blendv_ps(v_rel, v_atk, isAttackMask);

            // v_env = v_env + coeff * (inAbs - v_env)
            __m256 delta = _mm256_sub_ps(inAbs, v_env);
            v_env = _mm256_add_ps(v_env, _mm256_mul_ps(coeff, delta));

            // 2. dBへの高速変換
            __m256 envDb = _mm256_mul_ps(FastMath::fast_log2_256(v_env), db_scaler);

            // ==========================================
            // 3. Smart Safety Depth Control (水平Max抽出)
            // ==========================================
            // レジスタ内の6バンド全てから最も大きいdB値を抽出する
            __m256 t1 = _mm256_permute_ps(envDb, _MM_SHUFFLE(2, 3, 0, 1));
            __m256 m1 = _mm256_max_ps(envDb, t1);
            __m256 t2 = _mm256_permute_ps(m1, _MM_SHUFFLE(1, 0, 3, 2));
            __m256 m2 = _mm256_max_ps(m1, t2);
            __m256 t3 = _mm256_permute2f128_ps(m2, m2, 1);
            __m256 maxEnvDb = _mm256_max_ps(m2, t3); // 全レーンに最大値がブロードキャストされる

            // ゲート係数: clamp((maxEnvDb - (-90.0)) * 0.1, 0.0, 1.0)
            __m256 gateFactor = _mm256_mul_ps(_mm256_sub_ps(maxEnvDb, gate_floor), gate_scale);
            gateFactor = _mm256_max_ps(zero_ps, _mm256_min_ps(one_ps, gateFactor));

            // ==========================================
            // 4. コンプレッション量の計算
            // ==========================================
            // Downward (Threshold以上の超過分)
            __m256 overThresh = _mm256_max_ps(zero_ps, _mm256_sub_ps(envDb, down_thresh));
            __m256 downGainDb = _mm256_mul_ps(_mm256_mul_ps(overThresh, down_ratio_m), v_depth);

            // Upward (Threshold未満の不足分) - ここに安全ゲートを適用
            __m256 underThresh = _mm256_max_ps(zero_ps, _mm256_sub_ps(up_thresh, envDb));
            __m256 upGainDb = _mm256_mul_ps(_mm256_mul_ps(underThresh, up_ratio_m), v_depth);
            upGainDb = _mm256_mul_ps(upGainDb, gateFactor); // 無音時は増幅率が0になる

            // 5. リニアゲインへの復元と適用
            __m256 totalGainDb = _mm256_add_ps(downGainDb, upGainDb);
            __m256 linearGain = _mm256_mul_ps(FastMath::fast_exp2_256(_mm256_mul_ps(totalGainDb, linear_scaler)), v_gain);

            // 出力
            buffer[i] = _mm256_mul_ps(input, linearGain);
        }
    }

private:
    double currentSampleRate = 44100.0;

    // SIMD レジスタ状態変数
    __m256 v_env;
    __m256 v_atk;
    __m256 v_rel;
    __m256 v_depth;
    __m256 v_gain;

    float upwardThresholdDb = -40.0f;
    float downwardThresholdDb = -15.0f;
    float upwardRatio = 0.5f;
    float downwardRatio = 4.0f;
};