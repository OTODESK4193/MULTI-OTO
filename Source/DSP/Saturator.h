#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

class ADAASaturator {
public:
    ADAASaturator() = default;

    void reset() {
        x_prev = 0.0;
        F_prev = 0.0;
    }

    double processSample(double x, double driveAmount, double oddAmount, double evenAmount) {
        // Driveの適用 (0〜100 をゲイン倍率へ)
        double driveGain = 1.0 + (driveAmount * 0.2);
        double x_n = x * driveGain;

        // 倍音パラメータから係数を生成
        double a = 1.0;
        double b = oddAmount * 0.01;  // 正相の2次カーブ係数
        double c = 1.0;
        double d = evenAmount * 0.01; // 逆相の3次カーブ係数

        // 1. 積分関数 F(x) のブランチレス評価
        double x2 = x_n * x_n;
        double x3 = x2 * x_n;
        double x4 = x3 * x_n;

        double F_pos = (a * 0.5) * x2 - (b * 0.333333333333) * x3;
        double F_neg = (c * 0.5) * x2 + (d * 0.25) * x4;

        bool isPos = (x_n >= 0.0);
        double F_n = isPos ? F_pos : F_neg;

        // 2. 差分計算
        double delta_x = x_n - x_prev;
        double delta_F = F_n - F_prev;

        double out_y = 0.0;
        const double tolerance = 1e-9;

        // 3. Ill-Conditioning とゼロクロスオーバーの判定
        bool crossZero = (std::signbit(x_n) != std::signbit(x_prev));

        if (std::abs(delta_x) < tolerance || crossZero) {
            // 中点評価によるフォールバック近似
            double x_mid = (x_n + x_prev) * 0.5;
            double mid2 = x_mid * x_mid;
            double mid3 = mid2 * x_mid;

            double f_mid_pos = a * x_mid - b * mid2;
            double f_mid_neg = c * x_mid + d * mid3;

            out_y = (x_mid >= 0.0) ? f_mid_pos : f_mid_neg;
        }
        else {
            out_y = delta_F / delta_x;
        }

        x_prev = x_n;
        F_prev = F_n;

        return out_y / driveGain;
    }

private:
    double x_prev = 0.0;
    double F_prev = 0.0;
};