#include "ChiSquare.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <iostream>

// Вспомогательная функция для вычисления p-значения хи-квадрат
// Используется приближение через гамма-функцию (можно заменить на boost::math::gamma_p)
// Здесь реализована простая версия с использованием неполной гаммы из численных рецептов
// Для точности рекомендуется использовать Boost.Math.
namespace {
    // Неполная гамма-функция (нижняя) P(a,x) = 1/Γ(a) ∫_0^x t^{a-1} e^{-t} dt
    // Аппроксимация из Numerical Recipes
    double gammp(double a, double x) {
        const int MAXIT = 100;
        const double EPS = 1e-10;
        const double FPMIN = 1e-30;

        if (x < 0.0 || a <= 0.0) return 0.0;
        if (x == 0.0) return 0.0;

        // Используем ряды для x < a+1
        if (x < a + 1.0) {
            double ap = a;
            double sum = 1.0 / a;
            double del = sum;
            for (int n = 1; n <= MAXIT; ++n) {
                ap += 1.0;
                del *= x / ap;
                sum += del;
                if (std::abs(del) < std::abs(sum) * EPS) {
                    return sum * std::exp(-x + a * std::log(x) - std::lgamma(a));
                }
            }
        } else {
            // Продолженная дробь ( Lentz )
            double b = x + 1.0 - a;
            double c = 1.0 / FPMIN;
            double d = 1.0 / b;
            double h = d;
            for (int n = 1; n <= MAXIT; ++n) {
                double an = -n * (n - a);
                b += 2.0;
                d = an * d + b;
                if (std::abs(d) < FPMIN) d = FPMIN;
                c = b + an / c;
                if (std::abs(c) < FPMIN) c = FPMIN;
                d = 1.0 / d;
                double del = d * c;
                h *= del;
                if (std::abs(del - 1.0) < EPS) break;
            }
            double ans = std::exp(-x + a * std::log(x) - std::lgamma(a));
            return 1.0 - ans * h;
        }
        return 0.0;
    }
}

double ChiSquare::chiSquarePValue(const std::vector<double>& expected, const std::vector<long>& observed) {
    if (expected.size() != observed.size() || expected.empty())
        throw std::invalid_argument("Vectors must have same non-zero size");

    double chi2 = 0.0;
    for (size_t i = 0; i < expected.size(); ++i) {
        if (expected[i] <= 0) continue;
        double diff = static_cast<double>(observed[i]) - expected[i];
        chi2 += diff * diff / expected[i];
    }

    int df = static_cast<int>(expected.size()) - 1;
    if (df <= 0) return 1.0;

    // p-value = 1 - CDF(chi2, df)
    return 1.0 - gammp(0.5 * df, 0.5 * chi2);
}

void ChiSquare::processPixel(int value, int& nbBytes, int size,
                             std::vector<int>& hist, std::vector<double>& chi,
                             int& blockIdx, std::vector<double>& x) {
    hist[value]++;
    nbBytes++;
    if (nbBytes > size && blockIdx < static_cast<int>(chi.size())) {
        std::vector<double> expected(128, 0.0);
        std::vector<long> pov(128, 0);
        for (int k = 0; k < 128; ++k) {
            expected[k] = (hist[2*k] + hist[2*k+1]) / 2.0;
            pov[k] = hist[2*k];
        }
        chi[blockIdx] = chiSquarePValue(expected, pov);
        // x заполняется индексами (не используется)
        if (blockIdx < static_cast<int>(x.size()))
            x[blockIdx] = blockIdx;
        blockIdx++;
        nbBytes = 1;
        // hist не сбрасывается (в оригинале закомментировано)
    }
}

void ChiSquare::chiSquareAttackTopToBottom(const std::vector<std::vector<int>>& image,
                                           std::vector<double>& x,
                                           std::vector<double>& chi,
                                           int size) {
    int width = static_cast<int>(image.size());
    int height = (width > 0) ? static_cast<int>(image[0].size()) : 0;

    int blockIdx = 0;
    int nbBytes = 1;
    std::vector<int> hist(256, 1); // начальное значение 1 (как в Java)

    for (int y = 0; y < height; ++y) {
        for (int xx = 0; xx < width; ++xx) {
            if (blockIdx >= static_cast<int>(chi.size())) break;
            int val = image[xx][y];
            processPixel(val, nbBytes, size, hist, chi, blockIdx, x);
        }
    }
}

void ChiSquare::chiSquareAttackLeftToRight(const std::vector<std::vector<int>>& image,
                                           std::vector<double>& x,
                                           std::vector<double>& chi,
                                           int size) {
    int width = static_cast<int>(image.size());
    int height = (width > 0) ? static_cast<int>(image[0].size()) : 0;

    int blockIdx = 0;
    int nbBytes = 1;
    std::vector<int> hist(256, 1);

    for (int xx = 0; xx < width; ++xx) {
        for (int y = 0; y < height; ++y) {
            if (blockIdx >= static_cast<int>(chi.size())) break;
            int val = image[xx][y];
            processPixel(val, nbBytes, size, hist, chi, blockIdx, x);
        }
    }
}

void ChiSquare::chiSquareAttackBottomToTop(const std::vector<std::vector<int>>& image,
                                           std::vector<double>& x,
                                           std::vector<double>& chi,
                                           int size) {
    int width = static_cast<int>(image.size());
    int height = (width > 0) ? static_cast<int>(image[0].size()) : 0;

    int blockIdx = 0;
    int nbBytes = 1;
    std::vector<int> hist(256, 1);

    for (int y = height - 1; y >= 0; --y) {
        for (int xx = width - 1; xx >= 0; --xx) {
            if (blockIdx >= static_cast<int>(chi.size())) break;
            int val = image[xx][y];
            processPixel(val, nbBytes, size, hist, chi, blockIdx, x);
        }
    }
}

void ChiSquare::chiSquareAttackRightToLeft(const std::vector<std::vector<int>>& image,
                                           std::vector<double>& x,
                                           std::vector<double>& chi,
                                           int size) {
    int width = static_cast<int>(image.size());
    int height = (width > 0) ? static_cast<int>(image[0].size()) : 0;

    int blockIdx = 0;
    int nbBytes = 1;
    std::vector<int> hist(256, 1);

    for (int xx = width - 1; xx >= 0; --xx) {
        for (int y = 0; y < height; ++y) {
            if (blockIdx >= static_cast<int>(chi.size())) break;
            int val = image[xx][y];
            processPixel(val, nbBytes, size, hist, chi, blockIdx, x);
        }
    }
}