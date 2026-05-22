#include "RSAnalysis.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

RSAnalysis::RSAnalysis(int m, int n) : mM(m), mN(n) {
    // Создаём две маски размером m*n с чередующимися битами
    int total = m * n;
    mMask.resize(2, std::vector<int>(total, 0));

    int k = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            if (((j % 2) == 0 && (i % 2) == 0) || ((j % 2) == 1 && (i % 2) == 1)) {
                mMask[0][k] = 1;
                mMask[1][k] = 0;
            } else {
                mMask[0][k] = 0;
                mMask[1][k] = 1;
            }
            ++k;
        }
    }
}

int RSAnalysis::getPixelColour(int pixel, int /*colour*/) const {
    return pixel; // grayscale — просто значение пикселя
}

int RSAnalysis::negateLSB(int value) {
    int temp = value & 0xFE;          // обнуляем младший бит
    if (temp == value)                 // LSB был 0
        return value | 0x1;              // устанавливаем 1
    else                                 // LSB был 1
        return temp;                       // оставляем 0
}

int RSAnalysis::invertLSB(int value) {
    if (value == 255) return 256;
    if (value == 256) return 255;
    return negateLSB(value + 1) - 1;
}

std::vector<int> RSAnalysis::invertMask(const std::vector<int>& mask) {
    std::vector<int> inv(mask.size());
    for (size_t i = 0; i < mask.size(); ++i)
        inv[i] = -mask[i];
    return inv;
}

std::vector<int> RSAnalysis::flipBlock(const std::vector<int>& block, const std::vector<int>& mask) const {
    std::vector<int> flipped = block;
    for (size_t i = 0; i < block.size(); ++i) {
        if (mask[i] == 1) {
            int newVal = negateLSB(flipped[i]);
            flipped[i] = newVal & 0xFF;      // обрезка как в Java
        } else if (mask[i] == -1) {
            int newVal = invertLSB(flipped[i]);
            flipped[i] = newVal & 0xFF;
        }
    }
    return flipped;
}

double RSAnalysis::getVariation(const std::vector<int>& block, int colour) const {
    double var = 0.0;
    // Блок разбивается на четвёрки пикселей (если размер кратен 4)
    for (size_t i = 0; i + 3 < block.size(); i += 4) {
        int c0 = getPixelColour(block[i+0], colour);
        int c1 = getPixelColour(block[i+1], colour);
        int c2 = getPixelColour(block[i+2], colour);
        int c3 = getPixelColour(block[i+3], colour);

        var += std::abs(c0 - c1);
        var += std::abs(c1 - c3);
        var += std::abs(c3 - c2);
        var += std::abs(c2 - c0);
    }
    return var;
}

double RSAnalysis::getNegativeVariation(const std::vector<int>& block, int colour, const std::vector<int>& mask) const {
    double var = 0.0;
    for (size_t i = 0; i + 3 < block.size(); i += 4) {
        int c0 = getPixelColour(block[i+0], colour);
        int c1 = getPixelColour(block[i+1], colour);
        int c2 = getPixelColour(block[i+2], colour);
        int c3 = getPixelColour(block[i+3], colour);

        if (mask[i+0] == -1) c0 = invertLSB(c0);
        if (mask[i+1] == -1) c1 = invertLSB(c1);
        if (mask[i+2] == -1) c2 = invertLSB(c2);
        if (mask[i+3] == -1) c3 = invertLSB(c3);

        var += std::abs(c0 - c1);
        var += std::abs(c1 - c3);
        var += std::abs(c3 - c2);
        var += std::abs(c2 - c0);
    }
    return var;
}

std::vector<double> RSAnalysis::getAllPixelFlips(const std::vector<std::vector<int>>& image, int colour, bool overlap) const {
    // Создаём маску со всеми единицами
    std::vector<int> allmask(mM * mN, 1);

    int imgx = static_cast<int>(image.size());
    int imgy = (imgx > 0) ? static_cast<int>(image[0].size()) : 0;

    int startx = 0, starty = 0;
    std::vector<int> block(mM * mN);
    double numregular = 0, numsingular = 0;
    double numnegreg = 0, numnegsing = 0;
    // unusable не сохраняются, но нужны для подсчёта? В оригинале они есть, но в результатах не участвуют.
    double numunusable = 0, numnegunusable = 0;

    while (startx < imgx && starty < imgy) {
        for (int m = 0; m < 2; ++m) {
            // Заполняем блок
            int k = 0;
            for (int i = 0; i < mN; ++i) {
                for (int j = 0; j < mM; ++j) {
                    block[k] = image[startx + j][starty + i];
                    ++k;
                }
            }

            // Предварительно переворачиваем все пиксели (allmask)
            std::vector<int> blockFlippedAll = flipBlock(block, allmask);

            // Вариация после переворота всех пикселей
            double variationB = getVariation(blockFlippedAll, colour);

            // Применяем обычную маску
            std::vector<int> blockP = flipBlock(blockFlippedAll, mMask[m]);
            double variationP = getVariation(blockP, colour);

            // Негативная маска
            std::vector<int> maskNeg = invertMask(mMask[m]);
            double variationN = getNegativeVariation(blockFlippedAll, colour, maskNeg);

            // Классификация
            if (variationP > variationB) numregular++;
            if (variationP < variationB) numsingular++;
            if (variationP == variationB) numunusable++;

            if (variationN > variationB) numnegreg++;
            if (variationN < variationB) numnegsing++;
            if (variationN == variationB) numnegunusable++;
        }

        // Сдвиг окна
        if (overlap)
            startx += 1;
        else
            startx += mM;

        if (startx >= (imgx - 1)) {
            startx = 0;
            if (overlap)
                starty += 1;
            else
                starty += mN;
        }
        if (starty >= (imgy - 1))
            break;
    }

    return { numregular, numsingular, numnegreg, numnegsing };
}

double RSAnalysis::getX(double r, double rm, double r1, double rm1,
                        double s, double sm, double s1, double sm1) const {
    double d0 = r - s;
    double dm0 = rm - sm;
    double d1 = r1 - s1;
    double dm1 = rm1 - sm1;

    double a = 2.0 * (d1 + d0);
    double b = dm0 - dm1 - d1 - 3.0 * d0;
    double c = d0 - dm0;

    double x = 0.0;

    if (std::abs(a) < 1e-12) {
        // линейный случай
        if (std::abs(b) > 1e-12)
            x = c / b;
    } else {
        double discriminant = b * b - 4.0 * a * c;
        if (discriminant >= 0) {
            double root1 = (-b + std::sqrt(discriminant)) / (2.0 * a);
            double root2 = (-b - std::sqrt(discriminant)) / (2.0 * a);
            x = (std::abs(root1) <= std::abs(root2)) ? root1 : root2;
        } else {
            // fallback — прямая линия
            double cr = (rm - r) / (r1 - r + rm - rm1 + 1e-12);
            double cs = (sm - s) / (s1 - s + sm - sm1 + 1e-12);
            x = (cr + cs) / 2.0;
        }
    }

    if (std::abs(x) < 1e-12) {
        double cr = (rm - r) / (r1 - r + rm - rm1 + 1e-12);
        double cs = (sm - s) / (s1 - s + sm - sm1 + 1e-12);
        x = (cr + cs) / 2.0;
    }

    return x;
}

std::vector<double> RSAnalysis::doAnalysis(const std::vector<std::vector<int>>& image, int colour, bool overlap) {
    int imgx = static_cast<int>(image.size());
    int imgy = (imgx > 0) ? static_cast<int>(image[0].size()) : 0;

    int startx = 0, starty = 0;
    std::vector<int> block(mM * mN);
    double numregular = 0, numsingular = 0;
    double numnegreg = 0, numnegsing = 0;
    double numunusable = 0, numnegunusable = 0;
    double variationB, variationP, variationN;

    std::cout << "Image size: " << imgx << " x " << imgy << "\n";
    std::cout << "Total pixels: " << imgx * imgy << "\n";

    while (startx < imgx && starty < imgy) {
        for (int m = 0; m < 2; ++m) {
            // Чтение блока
            int k = 0;
            for (int i = 0; i < mN; ++i) {
                for (int j = 0; j < mM; ++j) {
                    block[k] = image[startx + j][starty + i];
                    ++k;
                }
            }

            variationB = getVariation(block, colour);

            // Положительная маска
            std::vector<int> blockP = flipBlock(block, mMask[m]);
            variationP = getVariation(blockP, colour);

            // Негативная маска
            std::vector<int> maskNeg = invertMask(mMask[m]);
            double variationN = getNegativeVariation(block, colour, maskNeg);

            // Классификация
            if (variationP > variationB) numregular++;
            if (variationP < variationB) numsingular++;
            if (variationP == variationB) numunusable++;

            if (variationN > variationB) numnegreg++;
            if (variationN < variationB) numnegsing++;
            if (variationN == variationB) numnegunusable++;
        }

        // Сдвиг
        if (overlap)
            startx += 1;
        else
            startx += mM;

        if (startx >= (imgx - 1)) {
            startx = 0;
            if (overlap)
                starty += 1;
            else
                starty += mN;
        }
        if (starty >= (imgy - 1))
            break;
    }

    double totalgroups = numregular + numsingular + numunusable;
    auto allpixels = getAllPixelFlips(image, colour, overlap);
    double x = getX(numregular, numnegreg, allpixels[0], allpixels[2],
                    numsingular, numnegsing, allpixels[1], allpixels[3]);

    double epf = 0.0, ml = 0.0;
    if (std::abs(2.0 * (x - 1.0)) > 1e-12)
        epf = std::abs(x / (2.0 * (x - 1.0)));
    if (std::abs(x - 0.5) > 1e-12)
        ml = std::abs(x / (x - 0.5));

    std::vector<double> results(28, 0.0);
    results[0] = numregular;
    results[1] = numsingular;
    results[2] = numnegreg;
    results[3] = numnegsing;
    results[4] = std::abs(numregular - numnegreg);
    results[5] = std::abs(numsingular - numnegsing);
    results[6] = (totalgroups > 0) ? (numregular / totalgroups) * 100.0 : 0;
    results[7] = (totalgroups > 0) ? (numsingular / totalgroups) * 100.0 : 0;
    results[8] = (totalgroups > 0) ? (numnegreg / totalgroups) * 100.0 : 0;
    results[9] = (totalgroups > 0) ? (numnegsing / totalgroups) * 100.0 : 0;
    results[10] = (totalgroups > 0) ? (results[4] / totalgroups) * 100.0 : 0;
    results[11] = (totalgroups > 0) ? (results[5] / totalgroups) * 100.0 : 0;

    results[12] = allpixels[0];
    results[13] = allpixels[1];
    results[14] = allpixels[2];
    results[15] = allpixels[3];
    results[16] = std::abs(allpixels[0] - allpixels[1]);
    results[17] = std::abs(allpixels[2] - allpixels[3]);
    results[18] = (totalgroups > 0) ? (allpixels[0] / totalgroups) * 100.0 : 0;
    results[19] = (totalgroups > 0) ? (allpixels[1] / totalgroups) * 100.0 : 0;
    results[20] = (totalgroups > 0) ? (allpixels[2] / totalgroups) * 100.0 : 0;
    results[21] = (totalgroups > 0) ? (allpixels[3] / totalgroups) * 100.0 : 0;
    results[22] = (totalgroups > 0) ? (results[16] / totalgroups) * 100.0 : 0;
    results[23] = (totalgroups > 0) ? (results[17] / totalgroups) * 100.0 : 0;

    results[24] = totalgroups;
    results[25] = epf;
    results[26] = ml * 100;
    results[27] = (static_cast<double>(imgx * imgy) * ml / 100.0) / 8.0; // для grayscale без учёта 3 каналов

std::cout << "Regular groups: " << numregular << "\n";
std::cout << "Singular groups: " << numsingular << "\n";
std::cout << "Neg regular: " << numnegreg << "\n";
std::cout << "Neg singular: " << numnegsing << "\n";
std::cout << "Total groups: " << totalgroups << "\n";

    return results;
}

std::vector<std::string> RSAnalysis::getResultNames() const {
    return {
        "Number of regular groups (positive)",
        "Number of singular groups (positive)",
        "Number of regular groups (negative)",
        "Number of singular groups (negative)",
        "Difference for regular groups",
        "Difference for singular groups",
        "Percentage of regular groups (positive)",
        "Percentage of singular groups (positive)",
        "Percentage of regular groups (negative)",
        "Percentage of singular groups (negative)",
        "Difference for regular groups %",
        "Difference for singular groups %",
        "Number of regular groups (positive for all flipped)",
        "Number of singular groups (positive for all flipped)",
        "Number of regular groups (negative for all flipped)",
        "Number of singular groups (negative for all flipped)",
        "Difference for regular groups (all flipped)",
        "Difference for singular groups (all flipped)",
        "Percentage of regular groups (positive for all flipped)",
        "Percentage of singular groups (positive for all flipped)",
        "Percentage of regular groups (negative for all flipped)",
        "Percentage of singular groups (negative for all flipped)",
        "Difference for regular groups (all flipped) %",
        "Difference for singular groups (all flipped) %",
        "Total number of groups",
        "Estimated percent of flipped pixels",
        "Estimated message length (in percent of pixels)(p)",
        "Estimated message length (in bytes)"
    };
}