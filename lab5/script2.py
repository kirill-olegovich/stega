import os
import re
import csv

pairs = [(c, real) for c in (2,3,4,5) for real in (2,3,4,5)]
out_file = "results.csv"

with open(out_file, 'w', newline='') as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(["c", "c_real", "prob"])
    for c, real in pairs:
        filename = f"results_c{c}_real{real}.txt"
        if not os.path.exists(filename):
            print(f"Файл {filename} не найден, пропускаем")
            continue
        with open(filename, 'r') as f:
            text = f.read()
        # Находим все числа после "its " и до "%"
        matches = re.findall(r'its\s+([0-9.]+)%', text)
        if not matches:
            print(f"В {filename} не найдено строк с процентами")
            continue
        probs = [float(x) for x in matches]
        mean_prob = sum(probs) / len(probs)
        writer.writerow([c, real, round(mean_prob, 2)])
        print(f"{c},{real} -> среднее = {mean_prob:.2f}% ({len(probs)} испытаний)")

print(f"Результаты сохранены в {out_file}")