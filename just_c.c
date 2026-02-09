#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// =============================================================
// SECTION 1: Color Image Structure
// =============================================================
typedef struct {
    int width;
    int height;
    float* r; // Red Channel
    float* g; // Green Channel
    float* b; // Blue Channel
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
        free(img->r);
        free(img->g);
        free(img->b);
        free(img);
    }
}

// =============================================================
// SECTION 2: BMP Helper Functions (Fixed & Color)
// =============================================================
ColorImage* read_bmp_color(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("Error: Cannot open file %s\n", filename);
        return NULL;
    }

    // 1. Read File Header (14 bytes)
    unsigned char fileHeader[14];
    if (fread(fileHeader, 1, 14, f) != 14) { fclose(f); return NULL; }

    // 2. Get Start of Pixel Data
    int dataOffset = *(int*)&fileHeader[10];

    // 3. Read Info Header (40 bytes)
    unsigned char infoHeader[40];
    if (fread(infoHeader, 1, 40, f) != 40) { fclose(f); return NULL; }

    int width = *(int*)&infoHeader[4];
    int height = *(int*)&infoHeader[8];

    // 4. Jump to Pixel Data
    fseek(f, dataOffset, SEEK_SET);

    int row_padded = (width * 3 + 3) & (~3);
    unsigned char* raw_data = (unsigned char*)malloc(row_padded * height);
    fread(raw_data, sizeof(unsigned char), row_padded * height, f);
    fclose(f);

    ColorImage* img = create_image(width, height);

    // Split BGR into R, G, B arrays
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int file_idx = (y * row_padded) + (x * 3);
            int pixel_idx = (y * width) + x;

            unsigned char b = raw_data[file_idx];
            unsigned char g = raw_data[file_idx + 1];
            unsigned char r = raw_data[file_idx + 2];

            img->b[pixel_idx] = (float)b;
            img->g[pixel_idx] = (float)g;
            img->r[pixel_idx] = (float)r;
        }
    }

    free(raw_data);
    return img;
}

void write_bmp_color(const char* filename, ColorImage* img) {
    FILE* f = fopen(filename, "wb");
    
    int w = img->width;
    int h = img->height;
    int row_padded = (w * 3 + 3) & (~3);
    int filesize = 54 + (row_padded * h); // 54 = 14 + 40 headers
    
    // Construct Headers
    unsigned char header[54] = {
        0x42, 0x4D,             // 'BM'
        0, 0, 0, 0,             // Filesize (filled later)
        0, 0, 0, 0,             // Reserved
        54, 0, 0, 0,            // Offset to data (standard 54)
        40, 0, 0, 0,            // Info header size
        0, 0, 0, 0,             // Width (filled later)
        0, 0, 0, 0,             // Height (filled later)
        1, 0, 24, 0,            // Planes & BPP
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    *(int*)&header[2] = filesize;
    *(int*)&header[18] = w;
    *(int*)&header[22] = h;

    fwrite(header, sizeof(unsigned char), 54, f);

    unsigned char* line_buffer = (unsigned char*)malloc(row_padded);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w) + x;
            
            // Clamp and Merge R, G, B
            float r = img->r[idx]; if(r<0) r=0; if(r>255) r=255;
            float g = img->g[idx]; if(g<0) g=0; if(g>255) g=255;
            float b = img->b[idx]; if(b<0) b=0; if(b>255) b=255;

            line_buffer[x*3]     = (unsigned char)b;
            line_buffer[x*3 + 1] = (unsigned char)g;
            line_buffer[x*3 + 2] = (unsigned char)r;
        }
        // Zero padding
        for(int k=w*3; k<row_padded; k++) line_buffer[k] = 0;
        fwrite(line_buffer, sizeof(unsigned char), row_padded, f);
    }

    free(line_buffer);
    fclose(f);
}

// =============================================================
// SECTION 3: Pure C Convolution Logic (Generic)
// =============================================================
// This function processes ONE channel (e.g., just Red).
// We will call it 3 times for a color image.
void convolution_c(float* input, float* output, int width, int height, float* kernel, int k_size) {
    int offset = k_size / 2;

    for (int y = offset; y < height - offset; y++) {
        for (int x = offset; x < width - offset; x++) {
            
            float sum = 0.0f;
            
            for (int ky = -offset; ky <= offset; ky++) {
                for (int kx = -offset; kx <= offset; kx++) {
                    float pixel = input[(y + ky) * width + (x + kx)];
                    float k_val = kernel[(ky + offset) * k_size + (kx + offset)];
                    sum += pixel * k_val;
                }
            }
            
            if (sum < 0) sum = 0;
            if (sum > 255) sum = 255;
            output[y * width + x] = sum;
        }
    }
}

// =============================================================
// SECTION 4: Main Execution
// =============================================================
int main() {
    printf("1. Loading 'input.bmp' (Color)...\n");
    ColorImage* input_img = read_bmp_color("lena.bmp");
    
    if (!input_img) return 1;
    printf("   Loaded: %d x %d pixels\n", input_img->width, input_img->height);

    ColorImage* output_img = create_image(input_img->width, input_img->height);

    // 3x3 Sharpen Kernel (Good for visuals)
    //  0 -1  0
    // -1  5 -1
    //  0 -1  0
    float kernel[9] = { 0, -1, 0, -1, 5, -1, 0, -1, 0 };

    // --- BENCHMARK PURE C ---
    printf("2. Running C Convolution (3 channels x 100 iterations)...\n");
    
    clock_t start = clock();
    int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Apply to Red
        convolution_c(input_img->r, output_img->r, input_img->width, input_img->height, kernel, 3);
        // Apply to Green
        convolution_c(input_img->g, output_img->g, input_img->width, input_img->height, kernel, 3);
        // Apply to Blue
        convolution_c(input_img->b, output_img->b, input_img->width, input_img->height, kernel, 3);
    }
    
    clock_t end = clock();
    
    // Stats
    double total_time = (double)(end - start) / CLOCKS_PER_SEC;
    double avg_time = total_time / iterations;
    
    printf("   Total Time: %.3f seconds\n", total_time);
    printf("   Average Time per Image: %.6f seconds\n", avg_time);

    // Save
    printf("3. Saving to 'output_c_pure.bmp'...\n");
    write_bmp_color("output_c_pure.bmp", output_img);

    free_image(input_img);
    free_image(output_img);
    printf("Done!\n");

    return 0;
}