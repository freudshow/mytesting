#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>

// Include the full structure definition from qwen-axdr.c
typedef struct AxdrBufferStruct {
    uint8_t *data;
    size_t size;
    size_t pos;
    size_t deepth;
    bool error;
    bool isEncodeTag;
    bool isEncodeLength;
    struct AxdrBufferStruct *children;
} AxdrBuffer;

// Forward declarations for the functions we need from qwen-axdr.c
AxdrBuffer* axdr_buffer_new_encoder(size_t initial_size);
AxdrBuffer* axdr_buffer_new_decoder(uint8_t *data, size_t size);
void axdr_buffer_free(AxdrBuffer *buf);
bool axdr_encode_integer_var(AxdrBuffer *buf, int64_t value);
bool axdr_decode_integer_var(AxdrBuffer *buf, int64_t *value);

// Test values as a struct for easier iteration
typedef struct {
    int64_t value;
    const char* description;
} TestValue;

void test_integer_var(void) {
    printf("\n=== Testing axdr_encode_integer_var ===\n\n");
    
    // Define test values
    TestValue test_values[] = {
        {0, "zero"},
        {-1, "minus one"},
        {-128, "minus 128"},
        {128, "128"},
        {32767, "32767 (max int16)"},
        {-32768, "minus 32768 (min int16)"},
        {2147483647, "2147483647 (max int32)"},
    };
    
    int test_count = sizeof(test_values) / sizeof(TestValue);
    int passed = 0;
    
    for (int i = 0; i < test_count; i++) {
        printf("Test case %d: %s (%" PRId64 ")\n", i + 1, test_values[i].description, test_values[i].value);
        
        // Create encoder buffer
        AxdrBuffer *enc_buf = axdr_buffer_new_encoder(64);
        if (!enc_buf) {
            printf("Failed to create encoder buffer\n");
            continue;
        }
        
        // Encode value
        bool encode_result = axdr_encode_integer_var(enc_buf, test_values[i].value);
        if (!encode_result) {
            printf("  FAILED: Encoding failed\n");
            axdr_buffer_free(enc_buf);
            continue;
        }
        
        // Print encoded bytes
        printf("  Encoded (%zu bytes): ", enc_buf->pos);
        for (size_t j = 0; j < enc_buf->pos; j++) {
            printf("%02X ", enc_buf->data[j]);
        }
        printf("\n");
        
        // Create decoder buffer
        AxdrBuffer *dec_buf = axdr_buffer_new_decoder(enc_buf->data, enc_buf->pos);
        if (!dec_buf) {
            printf("  FAILED: Could not create decoder buffer\n");
            axdr_buffer_free(enc_buf);
            continue;
        }
        
        // Decode value
        int64_t decoded_value;
        bool decode_result = axdr_decode_integer_var(dec_buf, &decoded_value);
        
        if (!decode_result) {
            printf("  FAILED: Decoding failed\n");
        } else if (decoded_value != test_values[i].value) {
            printf("  FAILED: Decoded value (%" PRId64 ") doesn't match original value (%" PRId64 ")\n",
                   decoded_value, test_values[i].value);
        } else {
            printf("  PASSED: Successfully encoded and decoded value\n");
            passed++;
        }
        
        // Cleanup
        axdr_buffer_free(enc_buf);
        axdr_buffer_free(dec_buf);
        printf("\n");
    }
    
    printf("=== Test Summary ===\n");
    printf("Total tests: %d\n", test_count);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", test_count - passed);
}

int testvarimain() {
    test_integer_var();
    return 0;
}
