import os
import cv2
import numpy as np
import tensorflow as tf
from tensorflow.keras import layers, models

# ==========================================
# 1. تنظیمات و بارگذاری دیتاست
# ==========================================
IMG_SIZE = 64
DATA_DIR = "mri_dataset"

def load_data():
    images = []
    labels = []
    
    # بارگذاری تصاویر سالم (برچسب 0)
    no_tumor_dir = os.path.join(DATA_DIR, "no_tumor")
    if os.path.exists(no_tumor_dir):
        for img_name in os.listdir(no_tumor_dir)[:500]: # استفاده از 200 عکس برای سرعت
            img_path = os.path.join(no_tumor_dir, img_name)
            img = cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)
            if img is not None:
                img = cv2.resize(img, (IMG_SIZE, IMG_SIZE))
                images.append(img)
                labels.append(0)

    # بارگذاری تصاویر دارای تومور (برچسب 1)
    tumor_dir = os.path.join(DATA_DIR, "tumor")
    if os.path.exists(tumor_dir):
        for img_name in os.listdir(tumor_dir)[:500]:
            img_path = os.path.join(tumor_dir, img_name)
            img = cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)
            if img is not None:
                img = cv2.resize(img, (IMG_SIZE, IMG_SIZE))
                images.append(img)
                labels.append(1)

    X = np.array(images).reshape(-1, IMG_SIZE, IMG_SIZE, 1) / 255.0
    y = np.array(labels)
    return X, y

print("Loading dataset...")
X, y = load_data()
print(f"Loaded {len(X)} images.")

# ==========================================
# 2. ساخت مدل میکرو CNN
# ==========================================
model = models.Sequential([
    # لایه کانولوشن: 4 فیلتر 3x3
    layers.Conv2D(4, (3, 3), activation='relu', input_shape=(IMG_SIZE, IMG_SIZE, 1), padding='valid'),
    # لایه ادغام (کاهش حجم)
    layers.MaxPooling2D((2, 2)),
    # تبدیل به وکتور یک بعدی
    layers.Flatten(),
    # لایه خروجی: 1 نورون با خروجی بین 0 و 1
    layers.Dense(1, activation='sigmoid')
])

model.compile(optimizer='adam', loss='binary_crossentropy', metrics=['accuracy'])

# ==========================================
# 3. آموزش مدل
# ==========================================
print("Training model...")
model.fit(X, y, epochs=10, batch_size=16, validation_split=0.2)

# ==========================================
# 4. استخراج وزن‌ها برای C / Assembly
# ==========================================
print("\nExporting weights for C/Assembly implementation...")

# دریافت وزن‌های لایه کانولوشن
conv_weights, conv_biases = model.layers[0].get_weights()
# shape conv_weights in Keras: (kernel_H, kernel_W, input_channels, output_filters)
# در اینجا: (3, 3, 1, 4)

with open("weights_conv.txt", "w") as f:
    for f_idx in range(4): # 4 فیلتر
        f.write(f"Filter_{f_idx}_Bias: {conv_biases[f_idx]}\n")
        for i in range(3):
            for j in range(3):
                f.write(f"{conv_weights[i, j, 0, f_idx]:.6f} ")
            f.write("\n")
        f.write("\n")

# دریافت وزن‌های لایه Dense
dense_weights, dense_biases = model.layers[3].get_weights()
with open("weights_dense.txt", "w") as f:
    f.write(f"Dense_Bias: {dense_biases[0]}\n")
    # نوشتن تمام وزن‌های اتصالی به صورت یک خط طولانی
    for w in dense_weights[:, 0]:
        f.write(f"{w:.6f}\n")

print("Weights successfully saved to 'weights_conv.txt' and 'weights_dense.txt'!")