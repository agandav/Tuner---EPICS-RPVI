//noise filtering test function
#include "../src/noise_filtering.c"

int main() {
    // test case: 20 samples with some noise spikes
    float input[20] = {
        1.0, 2.0, 3.0, 100.0, 5.0,    // spike at index 3 (value 100.0)
        6.0, 7.0, 8.0, 9.0, 10.0,
        11.0, -50.0, 13.0, 14.0, 15.0, // spike at index 11 (value -50.0)
        16.0, 17.0, 18.0, 19.0, 20.0
    };
    
    float output[20];
    
    // Apply median filter with window size 5
    median_filter(input, output, 20, 5);
    
    // Print results
    printf("Index | Input    | Output\n");
    printf("------|----------|--------\n");
    for (int i = 0; i < 20; i++) {
        printf("%5d | %8.2f | %8.2f\n", i, input[i], output[i]);
    }
    
    return 0;
}