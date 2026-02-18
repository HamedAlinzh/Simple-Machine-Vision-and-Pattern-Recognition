import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import scipy.stats as st
import os
from matplotlib.lines import Line2D
from matplotlib.patches import Patch

def run_statistical_analysis():
    if not os.path.exists("times.csv"):
        print("Error: 'times.csv' not found!")
        return

    # خواندن داده‌ها
    df = pd.read_csv("times.csv")
    time_c = df['Time_C'].values * 1000  # تبدیل ثانیه به میلی‌ثانیه
    time_asm = df['Time_ASM'].values * 1000

    print("\n" + "="*45)
    print(" 📊 STATISTICAL ANALYSIS REPORT (in milliseconds) ")
    print("="*45)

    mean_c, mean_asm = np.mean(time_c), np.mean(time_asm)
    std_c, std_asm = np.std(time_c), np.std(time_asm)

    ci_c = st.t.interval(0.95, len(time_c)-1, loc=mean_c, scale=st.sem(time_c))
    ci_asm = st.t.interval(0.95, len(time_asm)-1, loc=mean_asm, scale=st.sem(time_asm))

    print(f"Algorithm      | Mean Time  | Std Dev  | 95% Confidence Interval")
    print("-" * 65)
    print(f"Pure C (Slow)  | {mean_c:7.2f} ms | {std_c:7.2f} | [{ci_c[0]:.2f}, {ci_c[1]:.2f}]")
    print(f"Assembly SIMD  | {mean_asm:7.2f} ms | {std_asm:7.2f} | [{ci_asm[0]:.2f}, {ci_asm[1]:.2f}]")
    
    speedup = mean_c / mean_asm
    print("-" * 65)
    print(f"AVERAGE SPEEDUP: {speedup:.2f}x Faster")
    print("="*65)

    # ==========================================
    # رسم نمودار جعبه‌ای به همراه راهنمای علائم
    # ==========================================
    fig, ax = plt.subplots(figsize=(8, 6))
    
    data = [time_c, time_asm]
    labels = ['Pure C\n(Without SIMD)', 'Assembly\n(SIMD Optimized)']
    
    # استفاده از tick_labels به جای labels برای رفع اخطار ترمینال
    bplot = ax.boxplot(data, patch_artist=True, tick_labels=labels,
                       boxprops=dict(facecolor='#3498db', color='black'),
                       medianprops=dict(color='red', linewidth=2),
                       flierprops=dict(marker='o', color='gray', alpha=0.5))

    colors = ['#e74c3c', '#2ecc71']
    for patch, color in zip(bplot['boxes'], colors):
        patch.set_facecolor(color)

    ax.set_ylabel('Execution Time per Image (Milliseconds)', fontsize=12, fontweight='bold')
    ax.set_title('Statistical Distribution of Execution Times (100 Samples)', fontsize=14, fontweight='bold', pad=15)
    ax.yaxis.grid(True, linestyle='--', alpha=0.7)

    # ==========================================
    # ساخت باکس راهنما (Legend) برای توضیح علائم
    # ==========================================
    legend_elements = [
        Patch(facecolor='white', edgecolor='black', label='Box: 50% of Data (IQR)'),
        Line2D([0], [0], color='red', lw=2, label='Red Line: Median Time'),
        Line2D([0], [0], color='black', lw=1, label='Whiskers: Normal Range'),
        Line2D([0], [0], marker='o', color='w', markerfacecolor='w', markeredgecolor='gray', 
               markersize=8, label='Circles: Outliers (OS Delays)')
    ]
    
    # قرار دادن راهنما در بالا سمت راست (جایی که نمودار کاملاً خالی است)
    ax.legend(handles=legend_elements, loc='upper right', title="How to read this chart:", 
              framealpha=0.9, edgecolor='black', fontsize=10, title_fontsize=11)

    output_file = 'statistical_boxplot.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"\n✅ Statistical Boxplot saved as '{output_file}'!")

if __name__ == "__main__":
    run_statistical_analysis()