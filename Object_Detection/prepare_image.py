import cv2

def prepare_image_for_c(image_path, output_txt):
    print(f"Reading image: {image_path}")
    
    # 1. خواندن تصویر به صورت سیاه و سفید (Grayscale)
    img = cv2.imread(image_path, cv2.IMREAD_GRAYSCALE)
    if img is None:
        print("Error: Could not read image. Check the file path!")
        return
    
    # 2. تغییر ابعاد به 64x64 (دقیقا هم‌اندازه ورودی شبکه ما)
    img = cv2.resize(img, (64, 64))
    
    # 3. نرمال‌سازی داده‌ها (تبدیل اعداد 0 تا 255 به بازه 0 تا 1)
    img_normalized = img / 255.0
    
    # 4. ذخیره پیکسل‌ها در فایل متنی برای برنامه C
    with open(output_txt, 'w') as f:
        for y in range(64):
            for x in range(64):
                f.write(f"{img_normalized[y, x]:.6f} ")
            f.write("\n")
            
    print(f"Success! Image converted and saved to '{output_txt}'.")

# ==========================================
# آدرس یکی از عکس‌های دیتاست خود را اینجا بگذارید:
# برای تست تومور (خروجی باید نزدیک 1 باشد):
IMAGE_TO_TEST = "mri_dataset/no_tumor/Tr-no_1100.jpg"  # <-- اسم فایل را دقیق بنویسید

# برای تست مغز سالم (خروجی باید نزدیک 0 باشد):
# IMAGE_TO_TEST = "mri_dataset/no_tumor/Tr-no_0010.jpg" 

prepare_image_for_c(IMAGE_TO_TEST, "no_tumor_test_image.txt")