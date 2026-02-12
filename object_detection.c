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
// بخش ۴: تابع شناسایی مکان شیء (Max Location)
// =============================================================
void find_object_location(float* image_data, int width, int height, int img_id) {
    float max_val = -1.0f;
    int best_x = 0, best_y = 0;

    // گشتن در کل تصویر برای پیدا کردن روشن‌ترین نقطه (لبه مربع)
    for (int i = 0; i < width * height; i++) {
        if (image_data[i] > max_val) {
            max_val = image_data[i];
            best_y = i / width;
            best_x = i % width;
        }
    }
    
    // چاپ کردن ۱۰ مورد اول برای نمونه
    if (img_id < 10) {
        printf("   Image %d -> Detected Object at (X: %d, Y: %d)\n", img_id, best_x, best_y);
    }
}

// =============================================================
// بخش ۵: بدنه اصلی (Main Loop)
// =============================================================
int main() {
    int num_images = 100;
    char filename[100];
    
    // کرنل تشخیص لبه (برای پیدا کردن مربع)
    float kernel[9] = { 0, -1, 0, -1, 4, -1, 0, -1, 0 };

    printf("==========================================\n");
    printf(" STARTING PATTERN RECOGNITION BENCHMARK \n");
    printf("==========================================\n");

    // --- تست ۱: اجرای کند با C خالص ---
    printf("\n1. Running Pure C Loop (Slow)...\n");
    clock_t t1 = clock();
    
    for (int i = 0; i < num_images; i++) {
        sprintf(filename, "dataset/img_%d.bmp", i);
        ColorImage* img = read_bmp_dataset(filename);
        if (!img) { printf("Failed to read %s\n", filename); continue; }
        
        ColorImage* out = create_image(img->width, img->height);
        
        // فقط روی کانال قرمز اجرا می‌کنیم (چون تصویر سیاه و سفید است، کانال‌ها یکی هستند)
        convolution_pure_c(img->r, out->r, img->width, img->height, kernel);
        
        free_image(img);
        free_image(out);
    }
    clock_t t2 = clock();
    double time_c = ((double)(t2 - t1)) / CLOCKS_PER_SEC;
    printf("   Done! Total Time (C): %.4f seconds\n", time_c);


    // --- تست ۲: اجرای سریع با اسمبلی (SIMD) ---
    printf("\n2. Running Assembly SIMD Loop (Fast)...\n");
    clock_t t3 = clock();
    
    for (int i = 0; i < num_images; i++) {
        sprintf(filename, "dataset/img_%d.bmp", i);
        ColorImage* img = read_bmp_dataset(filename);
        if (!img) continue;
        
        ColorImage* out = create_image(img->width, img->height);
        
        // اجرا روی ۳ کانال (برای فشار بیشتر روی پردازنده و نمایش قدرت اسمبلی)
        apply_convolution_asm(img->r, out->r, img->width, img->height, kernel);
        apply_convolution_asm(img->g, out->g, img->width, img->height, kernel);
        apply_convolution_asm(img->b, out->b, img->width, img->height, kernel);
        
        // شناسایی مکان شیء (فقط یکبار در هر تصویر)
        find_object_location(out->r, img->width, img->height, i);

        free_image(img);
        free_image(out);
    }
    clock_t t4 = clock();
    double time_asm = ((double)(t4 - t3)) / CLOCKS_PER_SEC;
    
    printf("\n==========================================\n");
    printf(" FINAL RESULTS REPORT \n");
    printf("==========================================\n");
    printf("Number of Images: %d\n", num_images);
    printf("Pure C Time:      %.4f sec\n", time_c);
    printf("Assembly Time:    %.4f sec\n", time_asm);
    
    double speedup = time_c / time_asm;
    printf("SPEEDUP:          %.2fx Faster\n", speedup);
    printf("==========================================\n");

    return 0;
}