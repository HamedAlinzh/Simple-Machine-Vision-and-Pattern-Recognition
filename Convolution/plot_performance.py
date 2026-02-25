import matplotlib.pyplot as plt
import sys

# ==========================================
# ۱. دریافت داده‌ها از برنامه C
# ==========================================
# بررسی می‌کنیم که آیا برنامه C اعداد را فرستاده است یا خیر
if len(sys.argv) >= 3:
    time_pure_c = float(sys.argv[1])
    time_assembly = float(sys.argv[2])
else:
    print("Error: Please provide execution times.")
    print("Usage: python3 plot_performance.py <time_c> <time_asm>")
    sys.exit(1)

speedup = time_pure_c / time_assembly

print(f"Python received: C={time_pure_c:.4f}s, ASM={time_assembly:.4f}s. Drawing chart...")

# ==========================================
# ۲. تنظیمات نمودار (بقیه کد مثل قبل است)
# ==========================================
labels = ['Pure C (Slow)', 'Assembly SIMD (Fast)']
times = [time_pure_c, time_assembly]
colors = ['#e74c3c', '#2ecc71']

fig, ax = plt.subplots(figsize=(8, 6))
bars = ax.bar(labels, times, color=colors, width=0.5)
max_time = max(times)
ax.set_ylim(0, max_time * 1.2)
ax.set_ylabel('Execution Time (Seconds)', fontsize=12, fontweight='bold')
ax.set_title('Performance Comparison: C vs Assembly (100 Images)', fontsize=14, fontweight='bold', pad=20)
ax.yaxis.grid(True, linestyle='--', alpha=0.7)
ax.set_axisbelow(True)

for bar in bars:
    yval = bar.get_height()
    ax.text(bar.get_x() + bar.get_width()/2, yval + (max(times)*0.02), 
            f'{yval:.4f} s', ha='center', va='bottom', fontsize=11, fontweight='bold')

textstr = f'SPEEDUP\n{speedup:.2f}x Faster'
props = dict(boxstyle='round,pad=0.5', facecolor='#f1c40f', alpha=0.9, edgecolor='none')
ax.text(0.95, 0.95, textstr, transform=ax.transAxes, fontsize=14, fontweight='bold',
        verticalalignment='top', horizontalalignment='right', bbox=props, color='black')

output_file = 'performance_chart.png'
plt.savefig(output_file, dpi=300, bbox_inches='tight')
print(f"Chart successfully saved as '{output_file}'!")

# اگر می‌خواهید بعد از ساخت عکس، پنجره نمودار هم باز شود این خط را نگه دارید:
plt.show()