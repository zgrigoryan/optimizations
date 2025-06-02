import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("results.csv", names=["Unroll Factor", "Time (ns)"])
df = df.sort_values("Unroll Factor")

plt.figure(figsize=(8,5))
plt.plot(df["Unroll Factor"], df["Time (ns)"], marker='o')
plt.title("Copy Loop Timing vs Unroll Factor")
plt.xlabel("Unroll Factor")
plt.ylabel("Avg Time (ns)")
plt.grid(True)
plt.xticks(df["Unroll Factor"])
plt.tight_layout()
plt.savefig("loop_unrolling_plot.png", dpi=300)
plt.show()
