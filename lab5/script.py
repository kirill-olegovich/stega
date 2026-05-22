import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

df = pd.read_csv('results.csv')

pivot = df.pivot(index='c', columns='c_real', values='prob')

plt.figure(figsize=(6,5))
sns.heatmap(pivot, annot=True, fmt='.1f', cmap='RdYlGn', vmin=0, vmax=100,
            cbar_kws={'label': 'Вероятность обнаружения (%)'})
plt.xlabel('Реальный размер коалиции c_real')
plt.ylabel('Предполагаемый размер коалиции c')
plt.title('Вероятность обнаружения пиратов')
plt.tight_layout()
plt.savefig('heatmap.png', dpi=150)
plt.show()

pivot.plot(kind='bar', figsize=(8,5), colormap='viridis')
plt.xlabel('Предполагаемый c')
plt.ylabel('Вероятность обнаружения (%)')
plt.title('Вероятность обнаружения в зависимости от c и c_real')
plt.legend(title='c_real')
plt.ylim(0, 100)
plt.grid(axis='y', linestyle='--', alpha=0.7)
plt.tight_layout()
plt.savefig('barchart.png', dpi=150)
plt.show()