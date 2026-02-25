#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// بخش 1: تعریف تابع اسمبلی برای کانولوشن
extern void apply_convolution_asm(float* input, float* output, int width, int height, float* kernel);

// بخش 2: ساختار تصویر رنگی
// برای راحتی کار با تصاویر رنگی، ما یک ساختار جدید تعریف می‌کنیم که سه کانال R، G و B را نگه می‌دارد.
typedef struct {
    int width; // عرض تصویر
    int height; // ارتفاع تصویر
    float* r; // آرایه ای از اعداد اعشاری برای کانال قرمز
    float* g; // آرایه ای از اعداد اعشاری برای کانال سبز
    float* b; // آرایه ای از اعداد اعشاری برای کانال آبی
} ColorImage;

// تابعی برای ایجاد یک تصویر رنگی جدید با ابعاد مشخص
ColorImage* create_image(int w, int h) {
    ColorImage* img = (ColorImage*)malloc(sizeof(ColorImage));
    img->width = w;
    img->height = h;
    int size = w * h * sizeof(float); // حجم حافظه مورد نیاز برای هر کانال
    // رزرو حافظه برای هر کانال رنگی
    img->r = (float*)malloc(size);
    img->g = (float*)malloc(size);
    img->b = (float*)malloc(size);
    return img;
}

// تابعی برای آزادسازی حافظه اختصاص داده شده برای یک تصویر رنگی برای جلوگیری از memory leak
void free_image(ColorImage* img) {
    if(img) {
        free(img->r);
        free(img->g);
        free(img->b);
        free(img);
        // ابتدا بررسی می‌کنیم که img تهی نباشد، سپس حافظه اختصاص داده شده برای هر کانال را آزاد می‌کنیم و در نهایت خود ساختار تصویر را آزاد می‌کنیم. این کار بسیار مهم است تا از نشت حافظه جلوگیری کنیم.
    }
}

// بخش 3: توابع کمکی برای خواندن و نوشتن فایل‌های بی ام پی رنگی
ColorImage* read_bmp_color(const char* filename) {
    FILE* file = fopen(filename, "rb"); // باز کردن فایل برای خواندن به صورت باینری
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return NULL;
    }

    // 1. خواندن هدر فایل (14 بایت)
    unsigned char fileHeader[14];
    if (fread(fileHeader, 1, 14, file) != 14) {
        fclose(file);
        return NULL;
    }

    // 2. دریافت محل شروع داده‌های پیکسلی
    // این عملیات می گوید که پیکسل هایی که ما باید بخوانیم از کجا شروع می شوند. این مقدار در هدر فایل بی ام پی ذخیره شده است.
    int dataOffset = *(int*)&fileHeader[10];

    // 3. خواندن هدر اطلاعات (40 بایت)
    unsigned char infoHeader[40];
    if (fread(infoHeader, 1, 40, file) != 40) {
        fclose(file);
        return NULL;
    }
    // ذخیره عرض و ارتفاع تصویر 
    int width = *(int*)&infoHeader[4];
    int height = *(int*)&infoHeader[8];

    // 4. رفتن به محل داده‌های پیکسلی
    fseek(file, dataOffset, SEEK_SET);

    // تعداد بایت های هر ردیف باید مضرب 4 باشند
    int row_padded = (width * 3 + 3) & (~3);
    // حافظه مورد نیاز برای ذخیره داده‌های خام پیکسلی
    unsigned char* raw_data = (unsigned char*)malloc(row_padded * height);
    // خواندن پیکسل ها به صورت یکجا
    fread(raw_data, sizeof(unsigned char), row_padded * height, file);
    fclose(file);
    // ساخت تصویر خروجی
    ColorImage* img = create_image(width, height);
    // تبدیل فرمت بی آر جی به سه آرایه مجزای Float
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // محاسبه ایندکس پیکسل در داده‌های خام و ایندکس در آرایه‌های رنگی
            int file_index = (y * row_padded) + (x * 3);
            int pixel_index = (y * width) + x;
            
            unsigned char b = raw_data[file_index];
            unsigned char g = raw_data[file_index + 1];
            unsigned char r = raw_data[file_index + 2];
            // ذخیره رنگ ها به صورت اعداد اعشاری
            img->b[pixel_index] = (float)b;
            img->g[pixel_index] = (float)g;
            img->r[pixel_index] = (float)r;
        }
    }

    free(raw_data);
    return img;
}
// تابع ذخیره تصویر خروجی با فرمت استاندارد بی ام پی
void write_bmp_color(const char* filename, ColorImage* img) {
    FILE* file = fopen(filename, "wb");
    
    int w = img->width;
    int h = img->height;
    int row_padded = (w * 3 + 3) & (~3);
    int filesize = 54 + (row_padded * h); // حجم کل فایل = هدر (۵۴) + داده‌ها
    // هدر استاندارد ۵۴ بایتی برای فایل‌های بی ام پی
    unsigned char header[54] = {
        0x42, 0x4D, 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0, 40, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    // پر کردن مقادیر متغیر در هدر
    *(int*)&header[2] = filesize;
    *(int*)&header[18] = w;
    *(int*)&header[22] = h;

    fwrite(header, sizeof(unsigned char), 54, file);

    unsigned char* line_buffer = (unsigned char*)malloc(row_padded);
    // تبدیل دوباره اعداد اعشاری به بایت‌های رنگ (۰ تا ۲۵۵) و نوشتن در فایل
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w) + x;            
            float r = img->r[idx]; if(r<0) r=0; if(r>255) r=255;
            float g = img->g[idx]; if(g<0) g=0; if(g>255) g=255;
            float b = img->b[idx]; if(b<0) b=0; if(b>255) b=255;

            line_buffer[x*3] = (unsigned char)b;
            line_buffer[x*3 + 1] = (unsigned char)g;
            line_buffer[x*3 + 2] = (unsigned char)r;
        }
        for(int k=w*3; k<row_padded; k++) line_buffer[k] = 0;
        fwrite(line_buffer, sizeof(unsigned char), row_padded, file);
    }

    free(line_buffer);
    fclose(file);
}

// بخش 4: تابع اصلی برنامه
int main() {
    printf("1. Loading 'input.bmp' (Color)...\n");
    // بارگذاری تصویر ورودی با فرمت بی ام پی رنگی
    ColorImage* input_img = read_bmp_color("lena.bmp");
    
    if (!input_img) return 1;
    printf("   Loaded: %d x %d pixels\n", input_img->width, input_img->height);
    // ساخت تصویر خروجی با همان ابعاد تصویر ورودی
    ColorImage* output_img = create_image(input_img->width, input_img->height);

// تعریف فیلتر یا کرنل (اینجا فیلتر تشخیص لبه لاپلاسین انتخاب شده است)
    // [ 0, -1,  0 ]
    // [-1,  4, -1 ]
    // [ 0, -1,  0 ]
    float kernel[9] = { 
        1/9.0f, 1/9.0f, 1/9.0f,
        1/9.0f, 1/9.0f, 1/9.0f,
        1/9.0f, 1/9.0f, 1/9.0f
    };

// --- شروع پردازش کانولوشن با اسمبلی ---
    printf("2. Running ASM Convolution on R, G, B channels...\n");
    
    clock_t start = clock();
    int iterations = 100; // تعداد دفعات اجرا برای اندازه‌گیری دقیق سرعت
    
    for(int i=0; i<iterations; i++) {
        // تابع اسمبلی را ۳ بار صدا می‌زنیم: یک بار برای هر رنگ
        // چون منطق ریاضی کانولوشن برای قرمز، سبز و آبی یکسان است، نیازی به تغییر کد اسمبلی نیست.
        apply_convolution_asm(input_img->r, output_img->r, input_img->width, input_img->height, kernel);
        apply_convolution_asm(input_img->g, output_img->g, input_img->width, input_img->height, kernel);
        apply_convolution_asm(input_img->b, output_img->b, input_img->width, input_img->height, kernel);
    }
    clock_t end = clock();

    // محاسبه آمار و زمان اجرا
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("   Total Time (3 channels * %d runs): %f seconds\n", iterations, time_taken);
    printf("   Average Time per Color Image: %f seconds\n", time_taken / iterations);

    // ذخیره نتیجه نهایی
    printf("3. Saving to 'output_color.bmp'...\n");
    write_bmp_color("blur.bmp", output_img);

    free_image(input_img);
    free_image(output_img);
    return 0;
}