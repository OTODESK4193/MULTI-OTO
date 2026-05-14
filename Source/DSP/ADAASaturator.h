#pragma once
#include <juce_dsp/juce_dsp.h>
#include <immintrin.h>
#include <cmath>

class ADAASaturator {
public:
    ADAASaturator() { reset(); }

    void reset() {
        X_prev = _mm256_setzero_ps();
        F1_prev = _mm256_setzero_ps();
    }

    void processBlock_AVX2(float* channelData, int numSamples, float driveGain, float oddAmount, float evenAmount) {
        // --- 1. コントロールレート（チャンク単位）でのループ不変量の事前計算 ---
        const float d_lin = 1.0f + (driveGain * 0.2f);
        const float inv_d = 1.0f / d_lin;
        const float w_e = evenAmount * 0.01f;
        const float w_o = oddAmount * 0.01f;

        // クリップ境界 (L = 1.0) における Chebyshev多項式の定数値
        const float max_pos = 1.0f + 2.0f * (w_e + w_o);
        const float max_neg = -1.0f + 2.0f * w_e - 2.0f * w_o;

        // C0連続性を担保するための積分定数 C1 の厳密な導出
        const float F1_pos_1 = 0.5f - 0.4f * w_e - 0.3333333f * w_o;
        const float F1_neg_1 = 0.5f + 0.4f * w_e - 0.3333333f * w_o;
        const float C1_pos = F1_pos_1 - max_pos;
        const float C1_neg = F1_neg_1 + max_neg;

        // --- 2. SIMDレジスタへのブロードキャスト ---
        const __m256 drive = _mm256_set1_ps(d_lin);
        const __m256 inv_drive = _mm256_set1_ps(inv_d);
        const __m256 v_we = _mm256_set1_ps(w_e);
        const __m256 v_wo = _mm256_set1_ps(w_o);
        const __m256 v_max_p = _mm256_set1_ps(max_pos);
        const __m256 v_max_n = _mm256_set1_ps(max_neg);
        const __m256 v_c1_p = _mm256_set1_ps(C1_pos);
        const __m256 v_c1_n = _mm256_set1_ps(C1_neg);

        const __m256 eps_min = _mm256_set1_ps(1e-5f);
        const __m256 eps_rel = _mm256_set1_ps(1e-3f);
        const __m256 taylor_c = _mm256_set1_ps(0.041666666f); // 1/24
        const __m256 half = _mm256_set1_ps(0.5f);
        const __m256 limit = _mm256_set1_ps(1.0f);
        const __m256i sign_mask_i = _mm256_set1_epi32(0x7FFFFFFF);
        const __m256  sign_mask = _mm256_castsi256_ps(sign_mask_i);

        // --- 3. AVX2によるオーディオレート処理 ---
        int i = 0;
        for (; i <= numSamples - 8; i += 8) {
            processChunk(&channelData[i], drive, inv_drive, v_we, v_wo, v_max_p, v_max_n, v_c1_p, v_c1_n,
                eps_min, eps_rel, taylor_c, half, limit, sign_mask);
        }

        // バッファ末尾のフェイルセーフ（VST3の非8倍数バッファ長対応）
        if (i < numSamples) {
            alignas(32) float temp[8] = { 0 };
            for (int j = 0; j < numSamples - i; ++j) temp[j] = channelData[i + j];
            processChunk(temp, drive, inv_drive, v_we, v_wo, v_max_p, v_max_n, v_c1_p, v_c1_n,
                eps_min, eps_rel, taylor_c, half, limit, sign_mask);
            for (int j = 0; j < numSamples - i; ++j) channelData[i + j] = temp[j];
        }
    }

private:
    __m256 X_prev;
    __m256 F1_prev;

    // SIMD完全ブランチレス処理コア
    inline void processChunk(float* data, __m256 drive, __m256 inv_drive, __m256 w_e, __m256 w_o,
        __m256 max_p, __m256 max_n, __m256 c1_p, __m256 c1_n,
        __m256 eps_min, __m256 eps_rel, __m256 taylor_c, __m256 half,
        __m256 limit, __m256 sign_mask)
    {
        __m256 x_raw = _mm256_loadu_ps(data);
        __m256 X_n = _mm256_mul_ps(x_raw, drive); // Driven-domainでの評価

        // 動的閾値 (Driven Domain)
        __m256 delta_X = _mm256_sub_ps(X_n, X_prev);
        __m256 abs_delta_X = _mm256_and_ps(delta_X, sign_mask);
        __m256 abs_X_n = _mm256_and_ps(X_n, sign_mask);
        __m256 abs_X_prev = _mm256_and_ps(X_prev, sign_mask);

        __m256 max_abs_X = _mm256_max_ps(abs_X_n, abs_X_prev);
        __m256 dynamic_eps = _mm256_max_ps(eps_min, _mm256_mul_ps(eps_rel, max_abs_X));
        __m256 mask = _mm256_cmp_ps(abs_delta_X, dynamic_eps, _CMP_LT_OQ);

        // ==========================================
        // ルートA: ADAA1計算 (ΔF / ΔX)
        // ==========================================
        __m256 F1_n = eval_Chebyshev_F1(X_n, w_e, w_o, max_p, max_n, c1_p, c1_n, limit);
        __m256 delta_F = _mm256_sub_ps(F1_n, F1_prev);
        __m256 safe_delta_X = _mm256_blendv_ps(delta_X, _mm256_set1_ps(1.0f), mask);
        __m256 y_adaa = _mm256_div_ps(delta_F, safe_delta_X);

        // ==========================================
        // ルートB: 2次テイラー展開フォールバック
        // ==========================================
        __m256 X_mid = _mm256_mul_ps(_mm256_add_ps(X_n, X_prev), half);
        __m256 f_mid = eval_Chebyshev_f(X_mid, w_e, w_o, max_p, max_n, limit);

        __m256 delta_X_sq = _mm256_mul_ps(delta_X, delta_X);
        __m256 f_double_prime = eval_Chebyshev_f_double_prime(X_mid, w_e, w_o, limit, sign_mask);
        __m256 taylor_term = _mm256_mul_ps(_mm256_mul_ps(f_double_prime, delta_X_sq), taylor_c);
        __m256 y_fallback = _mm256_add_ps(f_mid, taylor_term);

        // ==========================================
        // ブランチレス合成と出力スケーリング (1/D)
        // ==========================================
        __m256 y_out = _mm256_blendv_ps(y_adaa, y_fallback, mask);
        __m256 final_out = _mm256_mul_ps(y_out, inv_drive);

        _mm256_storeu_ps(data, final_out);

        X_prev = X_n;
        F1_prev = F1_n;
    }

    inline __m256 eval_Chebyshev_f(__m256 X, __m256 w_e, __m256 w_o, __m256 max_p, __m256 max_n, __m256 L) {
        __m256 X2 = _mm256_mul_ps(X, X);
        __m256 X3 = _mm256_mul_ps(X2, X);
        __m256 X4 = _mm256_mul_ps(X3, X);
        __m256 X5 = _mm256_mul_ps(X4, X);

        __m256 we_poly = _mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(8.0f), X4), _mm256_mul_ps(_mm256_set1_ps(6.0f), X2));
        __m256 wo_poly = _mm256_add_ps(_mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(16.0f), X5), _mm256_mul_ps(_mm256_set1_ps(16.0f), X3)), _mm256_mul_ps(_mm256_set1_ps(2.0f), X));

        __m256 f_lin = _mm256_add_ps(X, _mm256_add_ps(_mm256_mul_ps(w_e, we_poly), _mm256_mul_ps(w_o, wo_poly)));

        __m256 mask_pos = _mm256_cmp_ps(X, L, _CMP_GT_OQ);
        __m256 mask_neg = _mm256_cmp_ps(X, _mm256_sub_ps(_mm256_setzero_ps(), L), _CMP_LT_OQ);

        __m256 res = _mm256_blendv_ps(f_lin, max_p, mask_pos);
        return _mm256_blendv_ps(res, max_n, mask_neg);
    }

    inline __m256 eval_Chebyshev_F1(__m256 X, __m256 w_e, __m256 w_o, __m256 max_p, __m256 max_n, __m256 c1_p, __m256 c1_n, __m256 L) {
        __m256 X2 = _mm256_mul_ps(X, X);
        __m256 X3 = _mm256_mul_ps(X2, X);
        __m256 X4 = _mm256_mul_ps(X3, X);
        __m256 X5 = _mm256_mul_ps(X4, X);
        __m256 X6 = _mm256_mul_ps(X5, X);

        __m256 t1 = _mm256_mul_ps(_mm256_set1_ps(0.5f), X2);
        __m256 we_poly = _mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(1.6f), X5), _mm256_mul_ps(_mm256_set1_ps(2.0f), X3));
        __m256 wo_poly = _mm256_add_ps(_mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(2.6666667f), X6), _mm256_mul_ps(_mm256_set1_ps(4.0f), X4)), X2);

        __m256 F1_lin = _mm256_add_ps(t1, _mm256_add_ps(_mm256_mul_ps(w_e, we_poly), _mm256_mul_ps(w_o, wo_poly)));
        __m256 F1_out_pos = _mm256_add_ps(_mm256_mul_ps(max_p, X), c1_p);
        __m256 F1_out_neg = _mm256_add_ps(_mm256_mul_ps(max_n, X), c1_n);

        __m256 mask_pos = _mm256_cmp_ps(X, L, _CMP_GT_OQ);
        __m256 mask_neg = _mm256_cmp_ps(X, _mm256_sub_ps(_mm256_setzero_ps(), L), _CMP_LT_OQ);

        __m256 res = _mm256_blendv_ps(F1_lin, F1_out_pos, mask_pos);
        return _mm256_blendv_ps(res, F1_out_neg, mask_neg);
    }

    inline __m256 eval_Chebyshev_f_double_prime(__m256 X, __m256 w_e, __m256 w_o, __m256 L, __m256 sign_mask) {
        __m256 X2 = _mm256_mul_ps(X, X);
        __m256 X3 = _mm256_mul_ps(X2, X);

        __m256 we_poly = _mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(96.0f), X2), _mm256_set1_ps(12.0f));
        __m256 wo_poly = _mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(320.0f), X3), _mm256_mul_ps(_mm256_set1_ps(96.0f), X));

        __m256 fdp_lin = _mm256_add_ps(_mm256_mul_ps(w_e, we_poly), _mm256_mul_ps(w_o, wo_poly));

        __m256 abs_X = _mm256_and_ps(X, sign_mask);
        __m256 mask_clip = _mm256_cmp_ps(abs_X, L, _CMP_GT_OQ);
        return _mm256_blendv_ps(fdp_lin, _mm256_setzero_ps(), mask_clip);
    }
};