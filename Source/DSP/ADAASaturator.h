#pragma once
#include <juce_dsp/juce_dsp.h>
#include <immintrin.h>
#include <cmath>

/**
 * ADAASaturator (C2 Continuous Polynomial Version - Final)
 * * 【修正内容】
 * ODD/Evenパラメータの変化に追従して、クリップする「高さ」と「積分定数(C1)」を
 * 動的に再計算するロジックを実装。
 * 波形が x=1.0 に到達した際、いかなるパラメータ設定下でも段差（ステップ関数）が
 * 生じず、完全に滑らかなC2連続としてクリップ領域に接続されます。
 * これにより、Drive 50以上の極限状態におけるディラック・インパルス（エイリアスの嵐）
 * を根絶します。
 */
class ADAASaturator {
public:
    ADAASaturator() { reset(); }

    void reset() {
        last_x = 0.0f;
        last_F1 = 0.0f;
    }

    /**
     * @param driveGain 0.0 ~ 100.0 (内部でスケーリング)
     * @param oddAmount 倍音バランス制御 (C2多項式の重み)
     * @param evenAmount 非対称性制御 (偶数次成分の重み)
     */
    void processBlock_AVX2(float* channelData, int numSamples, float driveGain, float oddAmount, float evenAmount) {
        // --- 1. コントロールレートでの事前計算 ---
        // 解析的ゲインスケーリング (1/D)
        const float d_lin = 1.0f + (driveGain * 0.2f);
        const float inv_d = 1.0f / d_lin;

        // パラメータの正規化
        const float w_o = 0.5f + (oddAmount * 0.005f);
        const float w_e = evenAmount * 0.01f;

        // 【最重要修正】積分定数 C1 の動的導出
        // x=1 における F1(x) の連続性を保証する
        // F1_poly(1) = 11/16 = 0.6875, F1_even(1) ≈ 0.457143
        // クリップ領域(x>=1)の高さは w_o となるため、積分は w_o * x + C_pos
        const float C1_pos = (-0.3125f * w_o) + (0.457143f * w_e);
        const float C1_neg = (-0.3125f * w_o) - (0.457143f * w_e);

        // --- 2. SIMDレジスタへのブロードキャスト ---
        const __m256 v_drive = _mm256_set1_ps(d_lin);
        const __m256 v_inv_d = _mm256_set1_ps(inv_d);
        const __m256 v_wo = _mm256_set1_ps(w_o);
        const __m256 v_we = _mm256_set1_ps(w_e);

        const __m256 v_c1_p = _mm256_set1_ps(C1_pos);
        const __m256 v_c1_n = _mm256_set1_ps(C1_neg);

        // 動的閾値とテイラー定数
        const __m256 eps_min = _mm256_set1_ps(1e-5f);
        const __m256 eps_rel = _mm256_set1_ps(1e-4f);
        const __m256 taylor_c = _mm256_set1_ps(0.5f); // 1/2

        // x軸上のクリップ境界 (L_bound = 1.0)
        const __m256 L_bound = _mm256_set1_ps(1.0f);
        const __m256i sign_mask_i = _mm256_set1_epi32(0x7FFFFFFF);
        const __m256  sign_mask = _mm256_castsi256_ps(sign_mask_i);

        const __m256i shift_idx = _mm256_setr_epi32(7, 0, 1, 2, 3, 4, 5, 6);

        int i = 0;
        for (; i <= numSamples - 8; i += 8) {
            processChunk(&channelData[i], v_drive, v_inv_d, v_wo, v_we, v_c1_p, v_c1_n,
                eps_min, eps_rel, taylor_c, L_bound, sign_mask, shift_idx, 8);
        }

        if (i < numSamples) {
            alignas(32) float temp[8] = { 0 };
            const int remainder = numSamples - i;
            for (int j = 0; j < remainder; ++j) temp[j] = channelData[i + j];
            processChunk(temp, v_drive, v_inv_d, v_wo, v_we, v_c1_p, v_c1_n,
                eps_min, eps_rel, taylor_c, L_bound, sign_mask, shift_idx, remainder);
            for (int j = 0; j < remainder; ++j) channelData[i + j] = temp[j];
        }
    }

private:
    float last_x;
    float last_F1;

    inline __m256 shift_and_blend(__m256 current, float prev_scalar, __m256i shift_idx) {
        __m256 shifted = _mm256_permutevar8x32_ps(current, shift_idx);
        __m256 prev_vec = _mm256_set1_ps(prev_scalar);
        return _mm256_blend_ps(shifted, prev_vec, 0x01);
    }

    inline void processChunk(float* data, __m256 drive, __m256 inv_drive, __m256 w_o, __m256 w_e,
        __m256 c1_p, __m256 c1_n, __m256 eps_min, __m256 eps_rel, __m256 taylor_c,
        __m256 L_bound, __m256 sign_mask, __m256i shift_idx, int validSamples)
    {
        __m256 x_raw = _mm256_loadu_ps(data);
        __m256 X_n = _mm256_mul_ps(x_raw, drive);

        __m256 X_prev = shift_and_blend(X_n, last_x, shift_idx);

        // 差分計算と動的閾値判定
        __m256 delta_X = _mm256_sub_ps(X_n, X_prev);
        __m256 abs_delta_X = _mm256_and_ps(delta_X, sign_mask);
        __m256 max_abs_X = _mm256_max_ps(_mm256_and_ps(X_n, sign_mask), _mm256_and_ps(X_prev, sign_mask));
        __m256 dynamic_eps = _mm256_max_ps(eps_min, _mm256_mul_ps(eps_rel, max_abs_X));
        __m256 mask = _mm256_cmp_ps(abs_delta_X, dynamic_eps, _CMP_LT_OQ);

        // ルートA: ADAA1 (ΔF / ΔX)
        __m256 F1_n = eval_C2_F1(X_n, w_o, w_e, c1_p, c1_n, L_bound);
        __m256 F1_prev = shift_and_blend(F1_n, last_F1, shift_idx);
        __m256 delta_F = _mm256_sub_ps(F1_n, F1_prev);
        __m256 safe_delta_X = _mm256_blendv_ps(delta_X, _mm256_set1_ps(1.0f), mask);
        __m256 y_adaa = _mm256_div_ps(delta_F, safe_delta_X);

        // ルートB: 2次テイラー展開フォールバック
        __m256 X_mid = _mm256_mul_ps(_mm256_add_ps(X_n, X_prev), _mm256_set1_ps(0.5f));
        __m256 f_mid = eval_C2_f(X_mid, w_o, w_e, L_bound);
        __m256 f_double_prime = eval_C2_f_double_prime(X_mid, w_o, w_e, L_bound, sign_mask);

        __m256 half_delta = _mm256_mul_ps(delta_X, _mm256_set1_ps(0.5f));
        __m256 taylor_term = _mm256_mul_ps(_mm256_mul_ps(f_double_prime, _mm256_mul_ps(half_delta, half_delta)), taylor_c);
        __m256 y_fallback = _mm256_add_ps(f_mid, taylor_term);

        // ブランチレス合成と出力スケーリング
        __m256 y_out = _mm256_blendv_ps(y_adaa, y_fallback, mask);
        __m256 final_out = _mm256_mul_ps(y_out, inv_drive);

        _mm256_storeu_ps(data, final_out);

        // 状態更新
        alignas(32) float x_n_arr[8];
        alignas(32) float F1_n_arr[8];
        _mm256_storeu_ps(x_n_arr, X_n);
        _mm256_storeu_ps(F1_n_arr, F1_n);
        last_x = x_n_arr[validSamples - 1];
        last_F1 = F1_n_arr[validSamples - 1];
    }

    /**
     * C2連続ソフトクリップ関数 f(x)
     */
    inline __m256 eval_C2_f(__m256 X, __m256 w_o, __m256 w_e, __m256 L_bound) {
        __m256 X2 = _mm256_mul_ps(X, X);

        // Horner's method: x * (1.875 - 1.25*x^2 + 0.375*x^4)
        __m256 poly = _mm256_add_ps(_mm256_set1_ps(1.875f),
            _mm256_mul_ps(X2, _mm256_add_ps(_mm256_set1_ps(-1.25f),
                _mm256_mul_ps(X2, _mm256_set1_ps(0.375f)))));
        __m256 f_poly = _mm256_mul_ps(X, poly);

        // 偶数次成分: (1-x^2)^3
        __m256 omx2 = _mm256_sub_ps(_mm256_set1_ps(1.0f), X2);
        __m256 f_even = _mm256_mul_ps(omx2, _mm256_mul_ps(omx2, omx2));

        __m256 f_mix = _mm256_add_ps(_mm256_mul_ps(f_poly, w_o), _mm256_mul_ps(f_even, w_e));

        __m256 mask_pos = _mm256_cmp_ps(X, L_bound, _CMP_GT_OQ);
        __m256 mask_neg = _mm256_cmp_ps(X, _mm256_sub_ps(_mm256_setzero_ps(), L_bound), _CMP_LT_OQ);

        // 【修正箇所】クリップ時の高さを 1.0f ではなく w_o と -w_o に連動させる
        __m256 res = _mm256_blendv_ps(f_mix, w_o, mask_pos);
        return _mm256_blendv_ps(res, _mm256_sub_ps(_mm256_setzero_ps(), w_o), mask_neg);
    }

    /**
     * 積分関数 F1(x)
     */
    inline __m256 eval_C2_F1(__m256 X, __m256 w_o, __m256 w_e, __m256 c1_p, __m256 c1_n, __m256 L_bound) {
        __m256 X2 = _mm256_mul_ps(X, X);

        // Odd component integral
        __m256 poly = _mm256_add_ps(_mm256_set1_ps(0.9375f),
            _mm256_mul_ps(X2, _mm256_add_ps(_mm256_set1_ps(-0.3125f),
                _mm256_mul_ps(X2, _mm256_set1_ps(0.0625f)))));
        __m256 F1_odd = _mm256_mul_ps(X2, poly);

        // Even component integral
        __m256 F1_even = _mm256_mul_ps(X, _mm256_add_ps(_mm256_set1_ps(1.0f),
            _mm256_mul_ps(X2, _mm256_add_ps(_mm256_set1_ps(-1.0f),
                _mm256_mul_ps(X2, _mm256_add_ps(_mm256_set1_ps(0.6f),
                    _mm256_mul_ps(X2, _mm256_set1_ps(-0.142857f))))))));

        __m256 F1_mix = _mm256_add_ps(_mm256_mul_ps(F1_odd, w_o), _mm256_mul_ps(F1_even, w_e));

        // 【修正箇所】クリップ領域の積分を w_o * x + C に修正
        __m256 w_o_neg = _mm256_sub_ps(_mm256_setzero_ps(), w_o);
        __m256 F1_out_pos = _mm256_add_ps(_mm256_mul_ps(w_o, X), c1_p);
        __m256 F1_out_neg = _mm256_add_ps(_mm256_mul_ps(w_o_neg, X), c1_n);

        __m256 mask_pos = _mm256_cmp_ps(X, L_bound, _CMP_GT_OQ);
        __m256 mask_neg = _mm256_cmp_ps(X, _mm256_sub_ps(_mm256_setzero_ps(), L_bound), _CMP_LT_OQ);

        __m256 res = _mm256_blendv_ps(F1_mix, F1_out_pos, mask_pos);
        return _mm256_blendv_ps(res, F1_out_neg, mask_neg);
    }

    /**
     * 二次微分 f''(x)
     */
    inline __m256 eval_C2_f_double_prime(__m256 X, __m256 w_o, __m256 w_e, __m256 L_bound, __m256 sign_mask) {
        __m256 X2 = _mm256_mul_ps(X, X);

        // Odd component f''
        __m256 fdp_odd = _mm256_mul_ps(_mm256_set1_ps(7.5f),
            _mm256_sub_ps(_mm256_mul_ps(X2, X), X));

        // Even component f''
        __m256 omx2 = _mm256_sub_ps(_mm256_set1_ps(1.0f), X2);
        __m256 fdp_even = _mm256_add_ps(_mm256_mul_ps(_mm256_set1_ps(-6.0f), _mm256_mul_ps(omx2, omx2)),
            _mm256_mul_ps(_mm256_set1_ps(24.0f), _mm256_mul_ps(X2, omx2)));

        __m256 fdp_mix = _mm256_add_ps(_mm256_mul_ps(fdp_odd, w_o), _mm256_mul_ps(fdp_even, w_e));

        __m256 abs_X = _mm256_and_ps(X, sign_mask);
        __m256 mask_clip = _mm256_cmp_ps(abs_X, L_bound, _CMP_GT_OQ);
        return _mm256_blendv_ps(fdp_mix, _mm256_setzero_ps(), mask_clip); // クリップ時は曲率0
    }
};