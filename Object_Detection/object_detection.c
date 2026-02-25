#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// =============================================================
// بخش ۱: تعریف تابع اسمبلی و ساختارها
// =============================================================
extern void apply_convolution_asm(float* input, float* output, int width, int height, float* kernel);

typedef struct {
    int width;
    int height;
    float* r; 
    float* g; 
    float* b; 
} ColorImage;

ColorImage* create_image(int w, int h) {
    ColorImage* img = (ColorImage*)malloc(sizeof(ColorImage));
    img->width = w;
    img->height = h;
    int size = w * h * sizeof(float);
    img->r = (float*)malloc(size);
    img->g = (float*)malloc(size);
    img->b = (float*)malloc(size);
    return img;
}

void free_image(ColorImage* img) {
    if(img) {
        free(img->r); free(img->g); free(img->b); free(img);
    }
}

// =============================================================
// بخش ۲: تابع خواندن BMP (بدون باگ Shift)
// =============================================================
ColorImage* read_bmp_dataset(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;

    unsigned char header[54];
    if (fread(header, 1, 54, f) != 54) { fclose(f); return NULL; }

    int dataOffset = *(int*)&header[10]; // آفست داده‌ها
    int width = *(int*)&header[18];
    int height = *(int*)&header[22];

    fseek(f, dataOffset, SEEK_SET); // پرش به شروع پیکسل‌ها

    int row_padded = (width * 3 + 3) & (~3);
    unsigned char* raw_data = (unsigned char*)malloc(row_padded * height);
    fread(raw_data, sizeof(unsigned char), row_padded * height, f);
    fclose(f);

    ColorImage* img = create_image(width, height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int file_idx = (y * row_padded) + (x * 3);
            int pixel_idx = (y * width) + x;
            
            img->b[pixel_idx] = (float)raw_data[file_idx];
            img->g[pixel_idx] = (float)raw_data[file_idx+1];
            img->r[pixel_idx] = (float)raw_data[file_idx+2];
        }
    }
    free(raw_data);
    return img;
}

// =============================================================
// بخش ۳: کانولوشن خالص C (برای مقایسه سرعت) - کند
// =============================================================
void convolution_pure_c(float* input, float* output, int width, int height, float* kernel) {
    int k_size = 3;
    int offset = 1;
    for (int y = offset; y < height - offset; y++) {
        for (int x = offset; x < width - offset; x++) {
            float sum = 0.0f;
            for (int ky = -offset; ky <= offset; ky++) {
                for (int kx = -offset; kx <= offset; kx++) {
                    float px = input[(y + ky) * width + (x + kx)];
                    float kv = kernel[(ky + offset) * k_size + (kx + offset)];
                    sum += px * kv;
                }
            }
            if (sum < 0) sum = 0;
            if (sum > 255) sum = 255;
            output[y * width + x] = sum;
        }
    }
}

// =============================================================
// بخش ۴: تابع شناسایی مکان و نوع شیء (Feature Extraction)
// =============================================================
void find_and_classify_object(float* orig_data, float* edge_data, int width, int height, int img_id, FILE* fp) {
    float max_val = -1.0f;
    int best_x = 0, best_y = 0;
    int area = 0;

    // ۱. پیدا کردن مکان با استفاده از خروجی اسمبلی (پیکسل‌های لبه)
    for (int i = 0; i < width * height; i++) {
        if (edge_data[i] > max_val) {
            max_val = edge_data[i];
            best_y = i / width;
            best_x = i % width;
        }
        
        // ۲. محاسبه مساحت با استفاده از عکس اصلی (تعداد پیکسل‌های سفید)
        if (orig_data[i] > 128.0f) {
            area++;
        }
    }
    
    // ۳. دسته‌بندی شکل بر اساس مساحت (Classification)
    const char* shape_name = "Unknown";
    if (area > 2200) {
        shape_name = "Square";      // مساحت حدود 2500
    } else if (area > 1600 && area <= 2200) {
        shape_name = "Circle";      // مساحت حدود 1963
    } else if (area > 1000 && area <= 1600) {
        shape_name = "Triangle";    // مساحت حدود 1250
    }

    if (img_id < 5) {
        printf("   Image %d -> Detected at (X: %d, Y: %d) | Shape: %s (Area: %d)\n", 
               img_id, best_x, best_y, shape_name, area);
    }
    
    // ذخیره در فایل CSV (اضافه شدن ستون شکل)
    fprintf(fp, "%d,%d,%d,%s\n", img_id, best_x, best_y, shape_name);
}

// =============================================================
// بخش ۵: بدنه اصلی (Main Loop) - نسخه آماری
// =============================================================
int main() {
    int num_images = 100;
    char filename[100];
    
    // آرایه‌هایی برای ذخیره زمان دقیق پردازش هر تصویر
    double time_c_array[100];
    double time_asm_array[100];
    
    float kernel[9] = { 0, -1, 0, -1, 4, -1, 0, -1, 0 };

    printf("==========================================\n");
    printf(" STARTING STATISTICAL BENCHMARK \n");
    printf("==========================================\n");

    // --- تست ۱: اجرای کند با C خالص ---
    printf("\n1. Running Pure C Loop (Slow)...\n");
    for (int i = 0; i < num_images; i++) {
        sprintf(filename, "dataset/img_%d.bmp", i);
        ColorImage* img = read_bmp_dataset(filename);
        if (!img) continue;
        
        ColorImage* out = create_image(img->width, img->height);
        
        clock_t t_start = clock(); // شروع زمان‌گیری برای این عکس
        convolution_pure_c(img->r, out->r, img->width, img->height, kernel);
        convolution_pure_c(img->g, out->g, img->width, img->height, kernel);
        convolution_pure_c(img->b, out->b, img->width, img->height, kernel);
        clock_t t_end = clock();   // پایان زمان‌گیری
        
        time_c_array[i] = ((double)(t_end - t_start)) / CLOCKS_PER_SEC;
        
        free_image(img);
        free_image(out);
    }
    printf("   Done!\n");

    // --- تست ۲: اجرای سریع با اسمبلی (SIMD) ---
    printf("\n2. Running Assembly SIMD Loop (Fast)...\n");
    FILE* fp_coords = fopen("coords.csv", "w");
    fprintf(fp_coords, "img_id,x,y,shape\n"); 
    
    for (int i = 0; i < num_images; i++) {
        sprintf(filename, "dataset/img_%d.bmp", i);
        ColorImage* img = read_bmp_dataset(filename);
        if (!img) continue;
        
        ColorImage* out = create_image(img->width, img->height);
        
        clock_t t_start = clock(); // شروع زمان‌گیری اسمبلی
        apply_convolution_asm(img->r, out->r, img->width, img->height, kernel);
        apply_convolution_asm(img->g, out->g, img->width, img->height, kernel);
        apply_convolution_asm(img->b, out->b, img->width, img->height, kernel);
        clock_t t_end = clock();   // پایان زمان‌گیری
        
        time_asm_array[i] = ((double)(t_end - t_start)) / CLOCKS_PER_SEC;
        
        find_and_classify_object(img->r, out->r, img->width, img->height, i, fp_coords);

        free_image(img);
        free_image(out);
    }
    fclose(fp_coords);
    printf("   Done!\n");

    // --- ذخیره زمان‌ها در فایل اکسل/CSV برای پایتون ---
    FILE* fp_times = fopen("times.csv", "w");
    fprintf(fp_times, "ImageID,Time_C,Time_ASM\n");
    for(int i = 0; i < num_images; i++) {
        fprintf(fp_times, "%d,%f,%f\n", i, time_c_array[i], time_asm_array[i]);
    }
    fclose(fp_times);
    
    printf("\nData saved to 'times.csv' and 'coords.csv'.\n");
    printf("Calling Python for Statistical Analysis...\n");
    
    // اجرای خودکار اسکریپت تحلیل آماری پایتون
    system("python3 analyze_stats.py");

    return 0;
}