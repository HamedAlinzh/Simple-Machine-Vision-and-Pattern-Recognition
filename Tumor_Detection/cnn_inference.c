#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// ابعاد شبکه
#define IMG_SIZE 64
#define FILTERS 4
#define K_SIZE 3
#define CONV_SIZE 62  // 64 - 3 + 1 = 62 (Valid Padding)
#define POOL_SIZE 31  // 62 / 2 = 31
#define FLATTEN_SIZE (FILTERS * POOL_SIZE * POOL_SIZE) // 3844

// متغیرهای ذخیره وزن‌ها
float conv_weights[FILTERS][K_SIZE][K_SIZE];
float conv_bias[FILTERS];
float dense_weights[FLATTEN_SIZE];
float dense_bias;

// ==========================================
// ۱. تابع بارگذاری وزن‌ها از فایل
// ==========================================
int load_weights() {
    FILE *f_conv = fopen("weights_conv.txt", "r");
    FILE *f_dense = fopen("weights_dense.txt", "r");
    
    if (!f_conv || !f_dense) {
        printf("Error: Could not open weight files!\n");
        return 0;
    }

    // خواندن وزن‌های کانولوشن
    char buffer[100];
    for (int f = 0; f < FILTERS; f++) {
        fscanf(f_conv, "%*s %f", &conv_bias[f]); // خواندن خط Bias
        for (int i = 0; i < K_SIZE; i++) {
            for (int j = 0; j < K_SIZE; j++) {
                fscanf(f_conv, "%f", &conv_weights[f][i][j]);
            }
        }
    }

    // خواندن وزن‌های لایه Dense
    fscanf(f_dense, "%*s %f", &dense_bias);
    for (int i = 0; i < FLATTEN_SIZE; i++) {
        fscanf(f_dense, "%f", &dense_weights[i]);
    }

    fclose(f_conv);
    fclose(f_dense);
    printf("Weights loaded successfully!\n");
    return 1;
}

// ==========================================
// ۲. تابع فعال‌ساز ReLU و Sigmoid
// ==========================================
float relu(float x) {
    return (x > 0) ? x : 0;
}

float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

// ==========================================
// ۳. توابع اصلی شبکه (C Pure - قابل تبدیل به اسمبلی)
// ==========================================
void conv2d_and_relu(float input[IMG_SIZE][IMG_SIZE], float output[FILTERS][CONV_SIZE][CONV_SIZE]) {
    for (int f = 0; f < FILTERS; f++) {
        for (int y = 0; y < CONV_SIZE; y++) {
            for (int x = 0; x < CONV_SIZE; x++) {
                float sum = conv_bias[f];
                for (int ky = 0; ky < K_SIZE; ky++) {
                    for (int kx = 0; kx < K_SIZE; kx++) {
                        sum += input[y + ky][x + kx] * conv_weights[f][ky][kx];
                    }
                }
                output[f][y][x] = relu(sum); // اعمال ReLU روی خروجی
            }
        }
    }
}

void max_pooling(float input[FILTERS][CONV_SIZE][CONV_SIZE], float output[FILTERS][POOL_SIZE][POOL_SIZE]) {
    for (int f = 0; f < FILTERS; f++) {
        for (int y = 0; y < POOL_SIZE; y++) {
            for (int x = 0; x < POOL_SIZE; x++) {
                // پیدا کردن بزرگترین عدد در یک مربع 2x2
                float max_val = input[f][y*2][x*2];
                if (input[f][y*2][x*2+1] > max_val) max_val = input[f][y*2][x*2+1];
                if (input[f][y*2+1][x*2] > max_val) max_val = input[f][y*2+1][x*2];
                if (input[f][y*2+1][x*2+1] > max_val) max_val = input[f][y*2+1][x*2+1];
                output[f][y][x] = max_val;
            }
        }
    }
}

float dense_layer(float input[FILTERS][POOL_SIZE][POOL_SIZE]) {
    float sum = dense_bias;
    int index = 0;
    
    // Flatten (تبدیل ماتریس 3 بعدی به 1 بعدی) و ضرب در وزن‌ها
    for (int y = 0; y < POOL_SIZE; y++) {
        for (int x = 0; x < POOL_SIZE; x++) {
            for (int f = 0; f < FILTERS; f++) {
                sum += input[f][y][x] * dense_weights[index];
                index++;
            }
        }
    }
    return sigmoid(sum);
}

// ==========================================
// تابع بارگذاری تصویر تست از فایل متنی
// ==========================================
int load_image(const char* filename, float image[IMG_SIZE][IMG_SIZE]) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Error: Could not open test_image.txt!\n");
        return 0;
    }
    for (int y = 0; y < IMG_SIZE; y++) {
        for (int x = 0; x < IMG_SIZE; x++) {
            fscanf(f, "%f", &image[y][x]);
        }
    }
    fclose(f);
    printf("Test image loaded successfully!\n");
    return 1;
}

int main() {
    // 1. بارگذاری مغز هوش مصنوعی (وزن‌ها)
    if (!load_weights()) return -1;

    // 2. بارگذاری عکس واقعی بیمار
    float test_image[IMG_SIZE][IMG_SIZE]; 
    if (!load_image("no_tumor_test_image.txt", test_image)) return -1;

    float conv_out[FILTERS][CONV_SIZE][CONV_SIZE] = {0};
    float pool_out[FILTERS][POOL_SIZE][POOL_SIZE] = {0};

    printf("\nRunning CNN Inference Engine...\n");
    
    // 3. اجرای لایه به لایه شبکه
    conv2d_and_relu(test_image, conv_out);
    max_pooling(conv_out, pool_out);
    float prediction = dense_layer(pool_out);

    // 4. چاپ نتیجه و درصد اطمینان
    printf("\n====================================\n");
    printf(" AI DIAGNOSIS REPORT \n");
    printf("====================================\n");
    
    // تبدیل عدد بین 0 و 1 به درصد
    float confidence = (prediction > 0.5) ? prediction * 100 : (1 - prediction) * 100;
    
    if (prediction > 0.5) {
        printf("RESULT:      TUMOR DETECTED (Glioma)\n");
    } else {
        printf("RESULT:      HEALTHY (No Tumor)\n");
    }
    
    printf("CONFIDENCE:  %.2f%%\n", confidence);
    printf("RAW SCORE:   %.6f\n", prediction);
    printf("====================================\n");

    return 0;
}