import numpy as np
import ctypes
import os
import time
import matplotlib.pyplot as plt

# 1. Load MNIST Dataset Locally
print("Loading MNIST dataset locally...")
with np.load('mnist.npz', allow_pickle=True) as f:
    x_train, y_train = f['x_train'], f['y_train']
    x_test, y_test = f['x_test'], f['y_test']

# Normalize and flatten the images (28x28 -> 784)
x_train = (x_train.reshape(x_train.shape[0], 784) / 255.0).astype(np.float32)
x_test = (x_test.reshape(x_test.shape[0], 784) / 255.0).astype(np.float32)

# 2. Create Templates (Average image for each digit)
print("Calculating digit templates (Training phase)...")
templates = np.zeros((10, 784), dtype=np.float32)
for i in range(10):
    digit_images = x_train[y_train == i]
    templates[i] = np.mean(digit_images, axis=0)

# 3. Load the C/Assembly Shared Library
lib_path = os.path.join(os.getcwd(), 'libclassifier.so')
if not os.path.exists(lib_path):
    print("Error: libclassifier.so not found. Run 'make' first.")
    exit(1)

classifier = ctypes.CDLL(lib_path)
classifier.predict_digit.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float)]
classifier.predict_digit.restype = ctypes.c_int

# 4. Pure Python Classifier Function (For Benchmark)
def predict_digit_python(img, templates):
    min_dist = float('inf')
    best_digit = -1
    for i in range(10):
        dist = np.sum((img - templates[i]) ** 2)
        if dist < min_dist:
            min_dist = dist
            best_digit = i
    return best_digit

# ==========================================
# PHASE 1: BENCHMARK (10,000 Images)
# ==========================================
num_test_images = len(x_test)
print(f"\n--- Phase 1: Benchmark on {num_test_images} test images ---")

print("Running Pure Python/NumPy version (Please wait)...")
correct_python = 0
start_time_python = time.time()
for j in range(num_test_images):
    pred = predict_digit_python(x_test[j], templates)
    if pred == y_test[j]:
        correct_python += 1
end_time_python = time.time()
time_python = end_time_python - start_time_python

print("Running C/Assembly SIMD version...")
correct_asm = 0
templates_ptr = templates.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
start_time_asm = time.time()
for j in range(num_test_images):
    img_ptr = x_test[j].ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    pred = classifier.predict_digit(img_ptr, templates_ptr)
    if pred == y_test[j]:
        correct_asm += 1
end_time_asm = time.time()
time_asm = end_time_asm - start_time_asm

acc_python = (correct_python / num_test_images) * 100
acc_asm = (correct_asm / num_test_images) * 100
speedup = time_python / time_asm if time_asm > 0 else float('inf')

print("\n" + "="*30)
print("       BENCHMARK RESULTS")
print("="*30)
print(f"Accuracy (Python): {acc_python:.2f}%")
print(f"Accuracy (C/ASM):  {acc_asm:.2f}%")
print(f"Time (Python):     {time_python:.4f} s")
print(f"Time (C/ASM):      {time_asm:.4f} s")
print(f"Speedup:           {speedup:.2f}x faster with ASM!")
print("="*30 + "\n")

# ==========================================
# PHASE 2: VISUAL PREDICTION (Single Image)
# ==========================================
print("--- Phase 2: Visual Prediction ---")
# Pick a random image from the test set
test_index = np.random.randint(0, num_test_images)
test_image = x_test[test_index]
actual_label = y_test[test_index]

# Pass it to Assembly code
img_ptr = test_image.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
predicted_label = classifier.predict_digit(img_ptr, templates_ptr)

print(f"Selected Image Index: {test_index}")
print(f"Actual Digit: {actual_label}")
print(f"Predicted Digit (by ASM): {predicted_label}")

# ==========================================
# PLOTTING
# ==========================================
# Create a figure with two subplots: one for the bar chart, one for the image
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

# Subplot 1: Benchmark Bar Chart
labels = ['Python (Numpy)', 'C + ASM (SIMD)']
times = [time_python, time_asm]
bars = ax1.bar(labels, times, color=['#e74c3c', '#2ecc71'])
ax1.set_ylabel('Time (Seconds)')
ax1.set_title(f'Performance Comparison\nSpeedup: {speedup:.2f}x')
for bar in bars:
    yval = bar.get_height()
    ax1.text(bar.get_x() + bar.get_width()/2, yval + (max(times)*0.02), f"{yval:.4f}s", ha='center', va='bottom', fontweight='bold')

# Subplot 2: Single Image Visual Prediction
ax2.imshow(test_image.reshape(28, 28), cmap='gray')
color = 'green' if predicted_label == actual_label else 'red'
ax2.set_title(f"ASM Prediction: {predicted_label}\n(Actual: {actual_label})", color=color, fontweight='bold')
ax2.axis('off')

plt.tight_layout()
plt.show()