#ifndef RSANALYSIS_H
#define RSANALYSIS_H

#include <vector>
#include <string>
#include <cmath>

class RSAnalysis {
public:
    // Константы для выбора цвета (оставлены для совместимости, но не используются)
    static const int ANALYSIS_COLOUR_RED = 0;
    static const int ANALYSIS_COLOUR_GREEN = 1;
    static const int ANALYSIS_COLOUR_BLUE = 2;

    /**
     * Конструктор.
     * @param m размер маски по x
     * @param n размер маски по y
     */
    RSAnalysis(int m, int n);

    /**
     * Выполняет RS-анализ изображения.
     * @param image матрица пикселей (grayscale, значения 0-255)
     * @param colour цвет (игнорируется, оставлен для совместимости)
     * @param overlap перекрываются ли блоки
     * @return вектор результатов (28 значений, см. getResultNames)
     */
    std::vector<double> doAnalysis(const std::vector<std::vector<int>>& image, int colour, bool overlap);

    /**
     * Возвращает названия результатов (для справки).
     */
    std::vector<std::string> getResultNames() const;

private:
    int mM;                     // ширина маски
    int mN;                     // высота маски
    std::vector<std::vector<int>> mMask; // две маски: mMask[0] и mMask[1]

    // Вспомогательные методы
    int getPixelColour(int pixel, int colour) const; // всегда возвращает pixel
    double getVariation(const std::vector<int>& block, int colour) const;
    double getNegativeVariation(const std::vector<int>& block, int colour, const std::vector<int>& mask) const;
    std::vector<int> flipBlock(const std::vector<int>& block, const std::vector<int>& mask) const;
    std::vector<double> getAllPixelFlips(const std::vector<std::vector<int>>& image, int colour, bool overlap) const;

    // LSB операции
    static int negateLSB(int value);   // меняет LSB (0→1, 1→0)
    static int invertLSB(int value);   // инвертирует LSB (с учётом переноса, как в Java)
    static std::vector<int> invertMask(const std::vector<int>& mask);

    // Расчёт x по формулам из статьи
    double getX(double r, double rm, double r1, double rm1,
                double s, double sm, double s1, double sm1) const;
};

#endif