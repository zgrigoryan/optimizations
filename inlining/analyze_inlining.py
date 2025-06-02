import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Load each result file and assign mode label
default_df = pd.read_csv("default_results.csv", names=["mode", "n", "time"])
forced_df = pd.read_csv("force_results.csv", names=["mode", "n", "time"])
noinline_df = pd.read_csv("noinline_results.csv", names=["mode", "n", "time"])

# Combine all into a single DataFrame
df = pd.concat([default_df, forced_df, noinline_df], ignore_index=True)

# Ensure correct types
df["n"] = df["n"].astype(int)
df["time"] = df["time"].astype(float)

# Summarize
summary = df.groupby("mode")["time"].agg(["mean", "min", "max", "std", "count"])
print("⏱️ Execution Time Summary (in seconds):")
print(summary)

# Plotting
plt.figure(figsize=(10, 6))
sns.barplot(data=df, x="mode", y="time", ci="sd", estimator="mean", palette="Set2")
plt.title("Average Execution Time by Inlining Strategy")
plt.ylabel("Time (seconds)")
plt.xlabel("Inlining Mode")
plt.grid(axis='y', linestyle='--', alpha=0.7)
plt.tight_layout()
plt.savefig("inlining_comparison.png", dpi=300)
plt.show()
