import cv2
import csv
import os
from PIL import Image

# ابعاد مربعی که در دیتاست ساختیم
OBJ_SIZE = 50 

def create_visual_results():
    print("Reading coordinates from C output (coords.csv)...")
    
    if not os.path.exists("coords.csv"):
        print("Error: 'coords.csv' not found. Run the C program first!")
        return

    frames = []
    
    # ساخت یک پوشه برای ذخیره عکس‌های علامت‌گذاری شده
    os.makedirs("annotated_results", exist_ok=True)

    with open("coords.csv", "r") as f:
        reader = csv.reader(f)
        next(reader) # رد کردن خط اول (هدر)
        
        for row in reader:
            img_id = int(row[0])
            x = int(row[1])
            y = int(row[2])
            shape = row[3] # خواندن اسم شکل از خروجی C
            
            img_path = f"dataset/img_{img_id}.bmp"
            img = cv2.imread(img_path)
            if img is None: continue

            y = img.shape[0] - y - OBJ_SIZE
            
            # تعیین رنگ کادر و متن بر اساس شکل
            if shape == "Square":
                color = (0, 0, 255)  # قرمز
                cv2.rectangle(img, (x, y), (x + OBJ_SIZE, y + OBJ_SIZE), color, 3)
                cv2.putText(img, shape, (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)
            elif shape == "Circle":
                color = (255, 0, 0)  # آبی
                cv2.rectangle(img, (x - 30, y - 5), (x + OBJ_SIZE - 20, y + OBJ_SIZE + 5), color, 3)
                cv2.putText(img, shape, (x - 25, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)
            elif shape == "Triangle":
                color = (0, 255, 0)  # سبز
                cv2.rectangle(img, (x - 50, y), (x + OBJ_SIZE - 50, y + OBJ_SIZE), color, 3)
                cv2.putText(img, shape, (x - 50, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)
            else:
                color = (255, 255, 255)


            # ذخیره تک‌تک عکس‌ها در پوشه (اختیاری)
            out_path = f"annotated_results/res_{img_id}.jpg"
            cv2.imwrite(out_path, img)

            # تبدیل رنگ از BGR به RGB برای ساخت GIF
            img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
            frames.append(Image.fromarray(img_rgb))

    # ==========================================
    # ساخت فایل متحرک (GIF) از تمام تصاویر
    # ==========================================
    print("Generating animated GIF...")
    if frames:
        # duration=150 یعنی هر عکس ۱۵۰ میلی‌ثانیه نمایش داده شود
        frames[0].save(
            "detection_animation.gif",
            save_all=True,
            append_images=frames[1:],
            duration=150,
            loop=0
        )
        print("SUCCESS! 'detection_animation.gif' has been created!")
        print("Check the 'annotated_results' folder for individual images.")

if __name__ == "__main__":
    create_visual_results()