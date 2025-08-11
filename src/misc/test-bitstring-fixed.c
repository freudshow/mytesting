#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

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
bool axdr_encode_bitstring_fixed(AxdrBuffer *buf, const uint8_t *bits, uint64_t bit_count);
bool axdr_decode_bitstring_fixed(AxdrBuffer *buf, uint8_t *bits, uint64_t *bit_count, uint64_t max_bits);
bool axdr_encode_bitstring_var(AxdrBuffer *buf, const uint8_t *bits, uint64_t bit_count);
bool axdr_decode_bitstring_var(AxdrBuffer *buf, const uint8_t *bits, uint64_t bits_buffer_size, int64_t *bit_count);

#define BITS_BUFFER_SIZE 1024       // Size of the bit buffer for testing
// Test values as a struct for easier iteration
typedef struct {
    uint8_t bits[BITS_BUFFER_SIZE]; // Input bit string bytes
    int64_t bit_count;              // Number of bits to encode
    const char* description;        // Description of the test case
} TestValue;

// Helper function to print bits
void print_bits(const uint8_t *data, int byte_count)

{
    for (int i = 0; i < byte_count; i++)
    {
        for (int j = 7; j >= 0; j--)
        {
            printf("%d", (data[i] >> j) & 1);
        }

        printf(" ");
    }
}

void test_bitstring_fixed(void)
{
    printf("\n=== Testing axdr_encode/decode_bitstring_fixed ===\n\n");
    
    // Define test values
    TestValue test_values[] = {
        {{0x80}, 1, "Single bit (1)"},                   // Just one bit set
        {{0x01}, 1, "Single bit (1 at end)"},            // Just one bit set at end
        {{0xFF}, 8, "All 8 bits set"},                   // All bits set
        {{0x00}, 8, "All 8 bits clear"},                 // All bits clear
        {{0xAA}, 8, "Alternating bits"},                 // Alternating pattern
        {{0xAA, 0x55}, 16, "16 bits alternating"},       // Two bytes alternating
        {{0xFF, 0xFF}, 12, "12 bits set"},               // 12 bits all set
        {{0xA5, 0x5A}, 16, "Complex pattern"}            // Complex pattern
    };
    
    int test_count = sizeof(test_values) / sizeof(TestValue);
    int passed = 0;

    for (int i = 0; i < test_count; i++)
    {
        printf("Test case %d: %s\n", i + 1, test_values[i].description);
        printf("  Input bits:  ");
        print_bits(test_values[i].bits, (test_values[i].bit_count + 7) / 8);
        printf("(%ld bits)\n", test_values[i].bit_count);

        // Create encoder buffer
        AxdrBuffer *enc_buf = axdr_buffer_new_encoder(64);
        if (!enc_buf)
        {
            printf("Failed to create encoder buffer\n");
            continue;
        }

        // Encode value
        bool encode_result = axdr_encode_bitstring_fixed(enc_buf, test_values[i].bits, test_values[i].bit_count);
        if (!encode_result)
        {
            printf("  FAILED: Encoding failed\n");
            axdr_buffer_free(enc_buf);
            continue;
        }

        // Print encoded bytes
        printf("  Encoded (%zu bytes): ", enc_buf->pos);
        for (size_t j = 0; j < enc_buf->pos; j++)
        {
            printf("%02X ", enc_buf->data[j]);
        }
        printf("\n");

        // Create decoder buffer
        AxdrBuffer *dec_buf = axdr_buffer_new_decoder(enc_buf->data, enc_buf->pos);
        if (!dec_buf)
        {
            printf("  FAILED: Could not create decoder buffer\n");
            axdr_buffer_free(enc_buf);
            continue;
        }

        // Decode value
        uint8_t decoded_bits[BITS_BUFFER_SIZE] = { 0 };  // Clear output buffer
        uint64_t decoded_bit_count = 0;
        bool decode_result = axdr_decode_bitstring_fixed(dec_buf, decoded_bits, &decoded_bit_count, test_values[i].bit_count);

        if (!decode_result)
        {
            printf("  FAILED: Decoding failed\n");
        }
        else
        {
            printf("  Decoded bits: ");
            print_bits(decoded_bits, (decoded_bit_count + 7) / 8);
            printf("(%ld bits)\n", decoded_bit_count);

            // Compare results
            bool match = (decoded_bit_count == test_values[i].bit_count);
            if (match)
            {
                // Compare only the bytes that contain valid bits
                for (int j = 0; j < test_values[i].bit_count; j++)
                {
                    int byte_index = j / 8;
                    int bit_index = 7 - (j % 8);  // MSB to LSB
                    if (((decoded_bits[byte_index] >> bit_index) & 0x01) != ((test_values[i].bits[byte_index] >> bit_index) & 0x01))
                    {
                        match = false;
                        break;
                    }
                }
            }

            if (!match)
            {
                printf("  FAILED: Decoded bits don't match input\n");
            }
            else
            {
                printf("  PASSED: Successfully encoded and decoded bits\n");
                passed++;
            }
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

void test_bitstring_var(void)
{
    printf("\n=== Testing axdr_encode/decode_bitstring variable ===\n\n");

    // Define test values
    TestValue test_values[] = {
            {{0x80}, 1, "Single bit (1)"},
            {{0x01}, 1, "Single bit (1 at end)"},
            {{0xFF}, 8, "All 8 bits set"},
            {{0x00}, 8, "All 8 bits clear"},
            {{0xAA}, 8, "Alternating bits"},
            {{0xAA, 0x55}, 16, "16 bits alternating"},
            {{0xFF, 0xFF}, 12, "12 bits set"},
            {{0xA5, 0x5A}, 16, "Complex pattern"},
            {{0xA5, 0x5A, 0xA5, 0x5A, 0xA5, 0x5A,
                    0xA5, 0x5A, 0xA5, 0x5A, 0xA5,
                    0x5A, 0xA5, 0x5A, 0xA5, 0x5A},
                    126, "126 bits set"},
            {{0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
                    0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
                    0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
                    0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
                    0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
                    0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
                    0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0xFF},
                    351, "350 bits"},
    };

    int test_count = sizeof(test_values) / sizeof(TestValue);
    int passed = 0;

    for (int i = 0; i < test_count; i++)
    {
        printf("Test case %d: %ld bits\n", i + 1, test_values[i].bit_count);
        printf("  Input bits:  ");
        print_bits(test_values[i].bits, (test_values[i].bit_count + 7) / 8);
        printf("(%ld bits)\n", test_values[i].bit_count);

        // Create encoder buffer
        AxdrBuffer *enc_buf = axdr_buffer_new_encoder(64);
        if (!enc_buf)
        {
            printf("Failed to create encoder buffer\n");
            continue;
        }

        // Encode value
        bool encode_result = axdr_encode_bitstring_var(enc_buf, test_values[i].bits, test_values[i].bit_count);
        if (!encode_result)
        {
            printf("  FAILED: Encoding failed\n");
            axdr_buffer_free(enc_buf);
            continue;
        }

        // Print encoded bytes
        printf("  Encoded (%zu bytes): ", enc_buf->pos);
        for (size_t j = 0; j < enc_buf->pos; j++)
        {
            printf("%02X ", enc_buf->data[j]);
        }
        printf("\n");

        // Create decoder buffer
        AxdrBuffer *dec_buf = axdr_buffer_new_decoder(enc_buf->data, enc_buf->pos);
        if (!dec_buf)
        {
            printf("  FAILED: Could not create decoder buffer\n");
            axdr_buffer_free(enc_buf);
            continue;
        }

        // Decode value
        uint8_t decoded_bits[BITS_BUFFER_SIZE] = { 0 }; // Clear output buffer
        int64_t decoded_bit_count = 0;
        bool decode_result = axdr_decode_bitstring_var(dec_buf, decoded_bits, sizeof(decoded_bits), &decoded_bit_count);

        if (!decode_result)
        {
            printf("  FAILED: Decoding %d-th failed\n", i + 1);
        }
        else
        {
            printf("  Decoded bits: ");
            print_bits(decoded_bits, (decoded_bit_count + 7) / 8);
            printf("(%ld bits)\n", decoded_bit_count);

            // Compare results
            bool match = (decoded_bit_count == test_values[i].bit_count);
            if (match)
            {
                // Compare only the bytes that contain valid bits
                for (int j = 0; j < test_values[i].bit_count; j++)
                {
                    int byte_index = j / 8;
                    int bit_index = 7 - (j % 8); // MSB to LSB
                    if (((decoded_bits[byte_index] >> bit_index) & 0x01) != ((test_values[i].bits[byte_index] >> bit_index) & 0x01))
                    {
                        match = false;
                        break;
                    }
                }
            }

            if (!match)
            {
                printf("  FAILED: Decoded bits don't match input\n");
            }
            else
            {
                passed++;
                printf("  PASSED: Successfully encoded and decoded bits\n");
            }
        }
    }

    printf("=== Test Summary ===\n");
    printf("Total tests: %d\n", test_count);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", test_count - passed);
}
