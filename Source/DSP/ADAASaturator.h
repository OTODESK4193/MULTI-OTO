#pragma once
#include <juce_dsp/juce_dsp.h>
#include <immintrin.h>
#include <cmath>

class ADAASaturator {
public:
    ADAASaturator() { reset(); }

    void reset() {
        x_prev = _mm256_setzero_ps();
        F1_prev = _mm256_setzero_ps();
    }

    // AVX2に最適化されたブロック一括処理（Block-based Processing）
    // バッファサイズが8の倍数でない場合（VST3仕様）にも末尾処理で安全に対応します。
    void processBlock_AVX2(float* channelData, int numSamples, float driveGain, float oddAmount, float evenAmount) {
        // 動的閾値とテイラー展開の定数
        const __m256 eps_min = _mm256_set1_ps(1e-5f);
        const __m256 eps_rel = _mm256_set1_ps(1e-3f);
        const __m256 taylor_c = _mm256_set1_ps(0.041666666f); // 1.0 / 24.0 (テイラー展開項)

        // Chebyshev倍音のウェイト
        const __m256 w_even = _mm256_set1_ps(evenAmount * 0.01f);
        const __m256 w_odd = _mm256_set1_ps(oddAmount * 0.01f);

        // 解析的ゲインスケーリング (Analytical Scale Optimization)
        // 内部で巨大化するスケールを圧縮し、32-bit Floatの仮数部情報落ちを防ぎます。
        float d_lin = 1.0f + (driveGain * 0.2f);
        const __m256 drive = _mm256_set1_ps(d_lin);
        const __m256 inv_drive = _mm256_set1_ps(1.0f / d_lin);
        const __m256 drive_sq = _mm256_set1_ps(d_lin * d_lin);

        const __m256 half = _mm256_set1_ps(0.5f);
        const __m256 limit = _mm256_set1_ps(1.0f); // Chebyshev定義域バウンド
        const __m256i sign_mask_i = _mm256_set1_epi32(0x7FFFFFFF);
        const __m256  sign_mask = _mm256_castsi256_ps(sign_mask_i);

        int i = 0;
        for (; i <= numSamples - 8; i += 8) {
            processChunk(&channelData[i], eps_min, eps_rel, drive, inv_drive, drive_sq,
                w_even, w_odd, half, taylor_c, limit, sign_mask);
        }

        // 8の倍数に満たないバッファ末尾の安全なフェイルセーフ処理
        if (i < numSamples) {
            alignas(32) float temp[8] = { 0 };
            for (int j = 0; j < numSamples - i; ++j) temp[j] = channelData[i + j];
            processChunk(temp, eps_min, eps_rel, drive, inv_drive, drive_sq,
                w_even, w_odd, half, taylor_c, limit, sign_mask);
            for (int j = 0; j < numSamples - i; ++j) channelData[i + j] = temp[j];
        }
    }

private:
    __m256 x_prev;
    __m256 F1_prev;

    // SIMDレジスタを用いた完全ブランチレスのADAAコア
    inline void processChunk(float* data, __m256 eps_min, __m256 eps_rel, __m256 drive, __m256 inv_drive,
        __m256 drive_sq, __m256 w_even, __m256 w_odd, __m256 half,
        __m256 taylor_c, __m256 limit, __m256 sign_mask)
    {
        __m256 x_n = _mm256_loadu_ps(data);

        __m256 delta_x = _mm256_sub_ps(x_n, x_prev);
        __m256 abs_delta_x = _mm256_and_ps(delta_x, sign_mask);
        __m256 abs_x_n = _mm256_and_ps(x_n, sign_mask);
        __m256 abs_x_prev = _mm256_and_ps(x_prev, sign_mask);

        // 動的閾値 (Dynamic Tolerance) の生成
        __m256 max_abs_x = _mm256_max_ps(abs_x_n, abs_x_prev);
        __m256 dynamic_eps = _mm256_max_ps(eps_min, _mm256_mul_ps(eps_rel, max_abs_x));
        __m256 mask = _mm256_cmp_ps(abs_delta_x, dynamic_eps, _CMP_LT_OQ);

        // ==========================================
        // ルートA: 通常のADAA1計算
        // ==========================================
        __m256 driven_x = _mm256_mul_ps(x_n, drive);
        __m256 F1_raw = eval_Chebyshev_F1(driven_x, w_even, w_odd, limit);
        __m256 F1_n = _mm256_mul_ps(F1_raw, inv_drive); // 解析的スケールダウン

        __m256 delta_F = _mm256_sub_ps(F1_n, F1_prev);
        __m256 safe_delta_x = _mm256_blendv_ps(delta_x, _mm256_set1_ps(1.0f), mask);
        __m256 y_adaa = _mm256_div_ps(delta_F, safe_delta_x);

        // ==========================================
        // ルートB: 2次テイラー展開フォールバック
        // ==========================================
        __m256 x_mid = _mm256_mul_ps(_mm256_add_ps(x_n, x_prev), half);
        __m256 driven_x_mid = _mm256_mul_ps(x_mid, drive);

        __m256 f_mid = eval_Chebyshev_f(driven_x_mid, w_even, w_odd, limit);
        __m256 delta_x_sq = _mm256_mul_ps(delta_x, delta_x);
        __m256 f_double_prime = eval_Chebyshev_f_double_prime(driven_x_mid, w_even, w_odd, limit, sign_mask);

        // 補正項: D^2 * f''(D x_mid) * (Δx^2 / 24)
        __m256 taylor_term = _mm256_mul_ps(_mm256_mul_ps(_mm256_mul_ps(f_double_prime, drive_sq), delta_x_sq), taylor_c);
        __m256 y_fallback = _mm256_add_ps(f_mid, taylor_term);

        // ==========================================
        // 分岐なし合成と出力スケーリング
        // ==========================================
        __m256 y_out = _mm256_blendv_ps(y_adaa, y_fallback, mask);
        __m256 final_out = _mm256_mul_ps(y_out, inv_drive); // 0dBアンカーポイントの維持

        _mm256_storeu_ps(data, final_out);

        x_prev = x_n;
        F1_prev = F1_n;
    }

    // Chebyshev合成関数 f(x) の評価
    inline __m256 eval_Chebyshev_f(__m256 x, __m256 w_e, __m256 w_o, __m256 L) {
        __m256 x2 = _mm256_mul_ps(x, x);
        __m256 x3 = _mm256_mul_ps(x2, x);
        __m256 x4 = _mm256_mul_ps(x3, x);
        __m256 x5 = _mm256_mul_ps(x4, x);

        __m256 we_poly = _mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(8.0f), x4), _mm256_mul_ps(_mm256_set1_ps(6.0f), x2));
        __m256 wo_poly = _mm256_add_ps(_mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(16.0f), x5), _mm256_mul_ps(_mm256_set1_ps(16.0f), x3)), _mm256_mul_ps(_mm256_set1_ps(2.0f), x));

        __m256 f_lin = _mm256_add_ps(x, _mm256_add_ps(_mm256_mul_ps(w_e, we_poly), _mm256_mul_ps(w_o, wo_poly)));

        // 定数クランプ (Limit)
        __m256 max_pos = _mm256_add_ps(_mm256_set1_ps(1.0f), _mm256_mul_ps(_mm256_set1_ps(2.0f), _mm256_add_ps(w_e, w_o)));
        __m256 max_neg = _mm256_sub_ps(_mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(2.0f), w_e), _mm256_mul_ps(_mm256_set1_ps(2.0f), w_o)), _mm256_set1_ps(1.0f));

        __m256 mask_pos = _mm256_cmp_ps(x, L, _CMP_GT_OQ);
        __m256 mask_neg = _mm256_cmp_ps(x, _mm256_sub_ps(_mm256_setzero_ps(), L), _CMP_LT_OQ);

        __m256 res = _mm256_blendv_ps(f_lin, max_pos, mask_pos);
        res = _mm256_blendv_ps(res, max_neg, mask_neg);
        return res;
    }

    // Chebyshev第一積分 F1(x) の評価と事前積分定数(C1)による連続性担保
    inline __m256 eval_Chebyshev_F1(__m256 x, __m256 w_e, __m256 w_o, __m256 L) {
        __m256 x2 = _mm256_mul_ps(x, x);
        __m256 x3 = _mm256_mul_ps(x2, x);
        __m256 x4 = _mm256_mul_ps(x3, x);
        __m256 x5 = _mm256_mul_ps(x4, x);
        __m256 x6 = _mm256_mul_ps(x5, x);

        __m256 c0_5 = _mm256_set1_ps(0.5f);
        __m256 t1 = _mm256_mul_ps(c0_5, x2);

        __m256 we_poly = _mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(1.6f), x5), _mm256_mul_ps(_mm256_set1_ps(2.0f), x3));
        __m256 wo_poly = _mm256_add_ps(_mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(2.6666667f), x6), _mm256_mul_ps(_mm256_set1_ps(4.0f), x4)), x2);

        __m256 F1_lin = _mm256_add_ps(t1, _mm256_add_ps(_mm256_mul_ps(w_e, we_poly), _mm256_mul_ps(w_o, wo_poly)));

        __m256 max_pos = _mm256_add_ps(_mm256_set1_ps(1.0f), _mm256_mul_ps(_mm256_set1_ps(2.0f), _mm256_add_ps(w_e, w_o)));
        __m256 max_neg = _mm256_sub_ps(_mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(2.0f), w_e), _mm256_mul_ps(_mm256_set1_ps(2.0f), w_o)), _mm256_set1_ps(1.0f));

        __m256 F1_pos_1 = _mm256_sub_ps(_mm256_sub_ps(c0_5, _mm256_mul_ps(_mm256_set1_ps(0.4f), w_e)), _mm256_mul_ps(_mm256_set1_ps(0.3333333f), w_o));
        __m256 C1_pos = _mm256_sub_ps(F1_pos_1, max_pos);
        __m256 F1_out_pos = _mm256_add_ps(_mm256_mul_ps(max_pos, x), C1_pos);

        __m256 F1_neg_1 = _mm256_sub_ps(_mm256_add_ps(c0_5, _mm256_mul_ps(_mm256_set1_ps(0.4f), w_e)), _mm256_mul_ps(_mm256_set1_ps(0.3333333f), w_o));
        __m256 C1_neg = _mm256_add_ps(F1_neg_1, max_neg);
        __m256 F1_out_neg = _mm256_add_ps(_mm256_mul_ps(max_neg, x), C1_neg);

        __m256 mask_pos = _mm256_cmp_ps(x, L, _CMP_GT_OQ);
        __m256 mask_neg = _mm256_cmp_ps(x, _mm256_sub_ps(_mm256_setzero_ps(), L), _CMP_LT_OQ);

        __m256 res = _mm256_blendv_ps(F1_lin, F1_out_pos, mask_pos);
        res = _mm256_blendv_ps(res, F1_out_neg, mask_neg);
        return res;
    }

    // Chebyshev第二導関数 f''(x) の評価
    inline __m256 eval_Chebyshev_f_double_prime(__m256 x, __m256 w_e, __m256 w_o, __m256 L, __m256 sign_mask) {
        __m256 x2 = _mm256_mul_ps(x, x);
        __m256 x3 = _mm256_mul_ps(x2, x);

        __m256 we_poly = _mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(96.0f), x2), _mm256_set1_ps(12.0f));
        __m256 wo_poly = _mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(320.0f), x3), _mm256_mul_ps(_mm256_set1_ps(96.0f), x));

        __m256 fdp_lin = _mm256_add_ps(_mm256_mul_ps(w_e, we_poly), _mm256_mul_ps(w_o, wo_poly));

        // 定数領域での曲率は0
        __m256 abs_x = _mm256_and_ps(x, sign_mask);
        __m256 mask_clip = _mm256_cmp_ps(abs_x, L, _CMP_GT_OQ);

        return _mm256_blendv_ps(fdp_lin, _mm256_setzero_ps(), mask_clip);
    }
};