#ifndef CHISQUARE_H
#define CHISQUARE_H

#include <vector>

class ChiSquare {
public:
    /**
     * Вычисляет p-значение хи-квадрат для наблюдаемых и ожидаемых частот.
     * @param expected ожидаемые частоты (должны быть >=0)
     * @param observed наблюдаемые частоты (целые)
     * @return p-значение (вероятность получить такую же или большую статистику при справедливой нулевой гипотезе)
     */
    static double chiSquarePValue(const std::vector<double>& expected, const std::vector<long>& observed);

    /**
     * Последовательный хи-квадрат анализ (сверху вниз).
     * @param image матрица пикселей (grayscale)
     * @param x массив, не используется (заполняется индексами для совместимости)
     * @param chi выходной массив p-значений
     * @param size размер выборки (количество пикселей для одного теста)
     */
    static void chiSquareAttackTopToBottom(const std::vector<std::vector<int>>& image,
                                           std::vector<double>& x,
                                           std::vector<double>& chi,
                                           int size);

    static void chiSquareAttackLeftToRight(const std::vector<std::vector<int>>& image,
                                           std::vector<double>& x,
                                           std::vector<double>& chi,
                                           int size);

    static void chiSquareAttackBottomToTop(const std::vector<std::vector<int>>& image,
                                           std::vector<double>& x,
                                           std::vector<double>& chi,
                                           int size);

    static void chiSquareAttackRightToLeft(const std::vector<std::vector<int>>& image,
                                           std::vector<double>& x,
                                           std::vector<double>& chi,
                                           int size);

private:
    // Вспомогательная функция для обработки одного пикселя (обновление гистограммы и проверка достижения size)
    static void processPixel(int value, int& nbBytes, int size,
                             std::vector<int>& hist, std::vector<double>& chi,
                             int& blockIdx, std::vector<double>& x);
};

#endif