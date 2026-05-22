#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import re
import numpy as np
from scipy import stats
import matplotlib.pyplot as plt

# ------------------ ПОРОГИ КЛАССИФИКАЦИИ ------------------
THRESHOLDS = {
    "RS": 5.0,       # % >5% -> stego
    "Chi2": 0.05,    # среднее p-value <0.05 -> stego
    "AUMP": 0.01     # beta >0.01 -> stego
}

# ------------------ ПАРСЕРЫ ДЛЯ ВАШИХ ФАЙЛОВ ------------------
def parse_rs(filepath):
    """Извлекает все проценты из p1.txt (строка 'Estimated message length (pixels %): X.XXXXX')"""
    values = []
    try:
        with open(filepath, 'r') as f:
            for line in f:
                m = re.search(r'Estimated message length \(pixels %\):\s*([\d\.]+)', line)
                if m:
                    values.append(float(m.group(1)))
    except Exception as e:
        print(f"Ошибка чтения {filepath}: {e}")
    return values

def parse_aump(filepath):
    """Извлекает все beta из p3.txt (строка 'Aump: X.XXXX')"""
    values = []
    try:
        with open(filepath, 'r') as f:
            for line in f:
                m = re.search(r'Aump:\s+([\d\.\-eE]+)', line)
                if m:
                    values.append(float(m.group(1)))
    except Exception as e:
        print(f"Ошибка чтения {filepath}: {e}")
    return values

def parse_chi2(filepath):
    """
    Извлекает из p2.txt блоки p-value.
    В вашем файле каждый блок начинается со строки "Chi-square p-values (top-to-bottom):"
    затем 25 строк с "block X: Y". Возвращает среднее p-value для каждого блока (изображения).
    """
    means = []
    try:
        with open(filepath, 'r') as f:
            lines = f.readlines()
        i = 0
        while i < len(lines):
            if "Chi-square p-values" in lines[i]:
                # Начало блока: следующие 25 строк содержат p-values
                pvals = []
                i += 1
                count = 0
                while count < 25 and i < len(lines):
                    m = re.search(r'block\s+\d+:\s+([\d\.eE\-]+)', lines[i])
                    if m:
                        pvals.append(float(m.group(1)))
                        count += 1
                    i += 1
                if pvals:
                    means.append(np.mean(pvals))
            else:
                i += 1
    except Exception as e:
        print(f"Ошибка чтения {filepath}: {e}")
    return means

# ------------------ ОСТАЛЬНЫЕ ФУНКЦИИ (те же, что и раньше) ------------------
def wilson_interval(k, n, conf=0.95):
    if n == 0:
        return (0.0, 0.0)
    p = k / n
    z = stats.norm.ppf(1 - (1 - conf) / 2)
    denom = 1 + z**2 / n
    centre = (p + z**2 / (2 * n)) / denom
    half_width = z * np.sqrt(p * (1 - p) / n + z**2 / (4 * n**2)) / denom
    return (centre - half_width, centre + half_width)

def evaluate(clean_vals, stego_vals, threshold):
    fp = sum(1 for v in clean_vals if v >= threshold)
    fn = sum(1 for v in stego_vals if v < threshold)
    n_clean = len(clean_vals)
    n_stego = len(stego_vals)
    type1_err = fp / n_clean if n_clean else 0
    type2_err = fn / n_stego if n_stego else 0
    return type1_err, type2_err, fp, n_clean, fn, n_stego

def compute_roc(clean_vals, stego_vals, n_thresholds=100):
    all_vals = np.concatenate([clean_vals, stego_vals])
    if len(all_vals) == 0:
        return [], []
    thresholds = np.linspace(np.min(all_vals), np.max(all_vals), n_thresholds)
    fpr_list, tpr_list = [], []
    for thr in thresholds:
        fp = sum(1 for v in clean_vals if v >= thr)
        tp = sum(1 for v in stego_vals if v >= thr)
        fpr = fp / len(clean_vals) if clean_vals else 0
        tpr = tp / len(stego_vals) if stego_vals else 0
        fpr_list.append(fpr)
        tpr_list.append(tpr)
    return fpr_list, tpr_list

# ------------------ ОСНОВНАЯ ЧАСТЬ ------------------
def main():
    base_dirs = {
        "clean": "results_orig",
        "stego_A": "results_A",
        "stego_B": "results_B"
    }

    # Проверка наличия папок
    for name, path in base_dirs.items():
        if not os.path.isdir(path):
            print(f"Папка '{path}' не найдена. Создайте её и положите туда p1.txt, p2.txt, p3.txt.")
            return

    # Чтение всех данных
    data = {}
    for name, path in base_dirs.items():
        data[name] = {
            "RS": parse_rs(os.path.join(path, "p1.txt")),
            "Chi2": parse_chi2(os.path.join(path, "p2.txt")),
            "AUMP": parse_aump(os.path.join(path, "p3.txt"))
        }
        print(f"{name}: RS={len(data[name]['RS'])}, Chi2={len(data[name]['Chi2'])}, AUMP={len(data[name]['AUMP'])}")

    # Анализ для двух типов стего
    for stego_name in ["stego_A", "stego_B"]:
        print("\n" + "="*70)
        print(f"АНАЛИЗ ДЛЯ МЕТОДА ВСТРАИВАНИЯ: {stego_name.upper()}")
        print("="*70)

        clean = data["clean"]
        stego = data[stego_name]

        for alg in ["RS", "Chi2", "AUMP"]:
            clean_vals = clean[alg]
            stego_vals = stego[alg]
            if len(clean_vals) == 0 or len(stego_vals) == 0:
                print(f"\n⚠️ {alg}: недостаточно данных (clean={len(clean_vals)}, stego={len(stego_vals)})")
                continue

            thr = THRESHOLDS[alg]
            err1, err2, fp, n_clean, fn, n_stego = evaluate(clean_vals, stego_vals, thr)
            ci1_low, ci1_high = wilson_interval(fp, n_clean)
            ci2_low, ci2_high = wilson_interval(fn, n_stego)

            print(f"\n--- {alg} (порог = {thr}) ---")
            print("Матрица ошибок:")
            print("                Предсказано")
            print("               Clean   Stego")
            print(f"  Реально Clean  {n_clean - fp:3d}      {fp:3d}")
            print(f"          Stego  {fn:3d}      {n_stego - fn:3d}")
            print(f"Ошибка I рода: {err1:.2%}  ДИ95% [{ci1_low:.2%}, {ci1_high:.2%}]")
            print(f"Ошибка II рода: {err2:.2%}  ДИ95% [{ci2_low:.2%}, {ci2_high:.2%}]")

        # ROC-кривые
        plt.figure(figsize=(8,6))
        for alg in ["RS", "Chi2", "AUMP"]:
            clean_vals = clean[alg]
            stego_vals = stego[alg]
            if len(clean_vals) == 0 or len(stego_vals) == 0:
                continue
            fpr, tpr = compute_roc(clean_vals, stego_vals)
            if len(fpr) == 0:
                continue
            auc = np.trapz(tpr, fpr)  # можно заменить на np.trapezoid
            plt.plot(fpr, tpr, label=f'{alg} (AUC={auc:.3f})', linewidth=2)
        plt.plot([0,1], [0,1], 'k--', label='Случайный')
        plt.xlabel('False Positive Rate')
        plt.ylabel('True Positive Rate')
        plt.title(f'ROC кривые для {stego_name.upper()}')
        plt.legend(loc='lower right')
        plt.grid(alpha=0.3)
        plt.savefig(f'roc_{stego_name}.png', dpi=150)
        plt.show()

if __name__ == "__main__":
    main()