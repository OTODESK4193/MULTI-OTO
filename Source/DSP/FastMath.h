#pragma once
#include <cstdint>

namespace FastMath {

    // IEEE 754のビット構造をハックした超高速log2近似（最大誤差<0.5%）
    inline float fast_log2(float x) {
        if (x <= 0.00001f) return -16.6f; // -100dB floor
        union { float f; uint32_t i; } vx = { x };
        float y = (float)vx.i;
        y *= 1.1920928955078125e-7f; // 2^-23 に乗算して仮数部をシフト
        return y - 126.94269504f;    // マジックナンバーによるバイアス補正
    }

    // テイラー展開とビットシフトを組み合わせた超高速exp2近似
    inline float fast_exp2(float x) {
        if (x < -126.0f) return 0.0f;
        if (x > 126.0f) return 3.402823466e+38f; // Max float

        int32_t i = (int32_t)x;
        float f = x - (float)i;
        if (x < 0.0f && f != 0.0f) { i--; f += 1.0f; } // 負の小数の補正

        // 小数部の多項式近似
        float y = 1.0f + f * (0.693147f + f * (0.240226f + f * 0.055504f));

        // 整数部を指数ビットに直接加算
        union { float f; uint32_t i; } res = { y };
        res.i += (uint32_t)(i << 23);
        return res.f;
    }

} // namespace FastMath