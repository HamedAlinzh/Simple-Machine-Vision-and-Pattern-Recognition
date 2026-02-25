#include <stdio.h>

// Declare the external assembly function
extern float calculate_distance_simd(float* img1, float* img2);

// Function to be called from Python
int predict_digit(float* test_img, float* templates) {
    int best_digit = -1;
    float min_distance = -1.0;

    for (int i = 0; i < 10; i++) {
        // templates is a 1D array conceptually treated as 10 arrays of 784 floats
        float* current_template = templates + (i * 784);
        
        // Call the highly optimized Assembly SIMD function
        float dist = calculate_distance_simd(test_img, current_template);
        
        if (best_digit == -1 || dist < min_distance) {
            min_distance = dist;
            best_digit = i;
        }
    }
    
    return best_digit;
}