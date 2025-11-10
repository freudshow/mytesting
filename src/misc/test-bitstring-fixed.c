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
bool axdr_decode_bitstring_fixed(AxdrBuffer *buf, uint8_t *bits, uint64_t *bit_count, int64_t max_bits);
bool axdr_encode_bitstring_var(AxdrBuffer *buf, const uint8_t *bits, uint64_t bit_count);
bool axdr_decode_bitstring_var(AxdrBuffer *buf, const uint8_t *bits, uint64_t bits_buffer_size, int64_t *bit_count);
bool axdr_encode_octetstring_fixed(AxdrBuffer *buf, const uint8_t *octets, int64_t octet_count);
bool axdr_decode_octetstring_fixed(AxdrBuffer *buf, uint8_t *octets, int64_t octet_count);
bool axdr_encode_octetstring_var(AxdrBuffer *buf, const uint8_t *octets, int64_t octet_count);
bool axdr_decode_octetstring_var(AxdrBuffer *buf, uint8_t **octets, int64_t *octet_count);

#define BITS_BUFFER_SIZE 1024       // Size of the bit buffer for testing
#define BYTES_BUFFER_SIZE BITS_BUFFER_SIZE  // Size of the byte string for testing

// Test values as a struct for easier iteration
typedef struct {
    uint8_t bits[BITS_BUFFER_SIZE]; // Input bit string bytes
    int64_t bit_count;              // Number of bits to encode
    const char *description;        // Description of the test case
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
                                { { 0x80 }, 1, "Single bit (1)" },                   // Just one bit set
            { { 0x01 }, 1, "Single bit (1 at end)" },            // Just one bit set at end
            { { 0xFF }, 8, "All 8 bits set" },                   // All bits set
            { { 0x00 }, 8, "All 8 bits clear" },                 // All bits clear
            { { 0xAA }, 8, "Alternating bits" },                 // Alternating pattern
            { { 0xAA, 0x55 }, 16, "16 bits alternating" },       // Two bytes alternating
            { { 0xFF, 0xFF }, 12, "12 bits set" },               // 12 bits all set
            { { 0xA5, 0x5A }, 16, "Complex pattern" }            // Complex pattern
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
                                { { 0x80 }, 1, "Single bit (1)" },
                                { { 0x01 }, 1, "Single bit (1 at end)" },
                                { { 0xFF }, 8, "All 8 bits set" },
                                { { 0x00 }, 8, "All 8 bits clear" },
                                { { 0xAA }, 8, "Alternating bits" },
                                { { 0xAA, 0x55 }, 16, "16 bits alternating" },
                                { { 0xFF, 0xFF }, 12, "12 bits set" },
                                { { 0xA5, 0x5A }, 16, "Complex pattern" },
                                { { 0xA5, 0x5A, 0xA5, 0x5A, 0xA5, 0x5A,
                                    0xA5,
                                    0x5A, 0xA5, 0x5A, 0xA5,
                                    0x5A,
                                    0xA5, 0x5A, 0xA5, 0x5A },
                                  126, "126 bits set" },
                                { { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
                                    0x55,
                                    0xAA, 0x55, 0xAA, 0x55, 0xAA,
                                    0x55,
                                    0xAA, 0x55, 0xAA, 0x55, 0xAA,
                                    0x55,
                                    0xAA, 0x55, 0xAA, 0x55, 0xAA,
                                    0x55,
                                    0xAA, 0x55, 0xAA, 0x55, 0xAA,
                                    0x55,
                                    0xAA, 0x55, 0xAA, 0x55, 0xAA,
                                    0x55,
                                    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0xFF },
                                  351, "350 bits" },
                                { { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
                                    0x55,
                                    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
                                    0xAA,
                                    0x55 },
                                  128, "128 bits" },
                                { { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
                                    0x55,
                                    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
                                    0xAA,
                                    0x55 },
                                  127, "127 bits" },
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

        // Cleanup
        axdr_buffer_free(enc_buf);
        axdr_buffer_free(dec_buf);
    }

    printf("=== Test Summary ===\n");
    printf("Total tests: %d\n", test_count);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", test_count - passed);
}

typedef struct TestOctetString {
    uint8_t octet[BYTES_BUFFER_SIZE];   // Buffer to hold the byte string data
    int64_t octet_count;               // Number of octets to encode
    const char *description;            // Description of the test case
} TestOctetString_s;

void testOctetStringFixed(void)
{
    TestOctetString_s testTable[] =
            {
              { { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }, 8, "8 Octet String" },
              { { 0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9, 0xF8 }, 8, "8 Octet String (Reverse)" },
              { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 }, 8, "8 Octet String (Zero Start)" },
              { { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }, 6, "6 Octet String" },
              { { }, 0, "Empty Octet String" }, // Edge case: empty string
              { { 0x01 }, 1, "Single Octet String" }, // Edge case: single octet
            };

    int test_count = sizeof(testTable) / sizeof(TestOctetString_s);
    int passed = 0;

    for (int i = 0; i < test_count; i++)
    {
        printf("Test case %d: %s\n", i + 1, testTable[i].description);
        printf("  Input octets: ");

        // Print the octet string
        for (uint64_t j = 0; j < testTable[i].octet_count; j++)
        {
            printf("%02X ", testTable[i].octet[j]);
        }
        printf("(%ld octets)\n", testTable[i].octet_count);

        // Create encoder buffer
        AxdrBuffer *enc_buf = axdr_buffer_new_encoder(64);
        if (!enc_buf)
        {
            printf("Failed to create encoder buffer\n");
            continue;
        }

        printf("  Encoding octet string...\n");
        // Encode value
        bool encode_result = axdr_encode_octetstring_fixed(enc_buf, testTable[i].octet, testTable[i].octet_count);
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
        uint8_t decoded_octets[BYTES_BUFFER_SIZE] = { 0 }; // Clear output buffer
        bool decode_result = axdr_decode_octetstring_fixed(dec_buf, decoded_octets, testTable[i].octet_count);
        if (!decode_result)
        {
            printf("  FAILED: Decoding failed\n");
        }
        else
        {
            printf("  Decoded octets: ");
            for (uint64_t j = 0; j < testTable[i].octet_count; j++)
            {
                printf("%02X ", decoded_octets[j]);
            }
            printf("(%ld octets)\n", testTable[i].octet_count);

            // Compare results
            bool match = true;
            for (uint64_t j = 0; j < testTable[i].octet_count; j++)
            {
                if (decoded_octets[j] != testTable[i].octet[j])
                {
                    match = false;
                    break;
                }
            }

            if (!match)
            {
                printf("  FAILED: Decoded octets don't match input\n");
            }
            else
            {
                passed++;
                printf("  PASSED: Successfully encoded and decoded octet string\n");
            }
        }

        // Cleanup
        axdr_buffer_free(enc_buf);
        axdr_buffer_free(dec_buf);
    }

    printf("=== Test Summary ===\n");
    printf("Total tests: %d\n", test_count);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", test_count - passed);
}

void testOctetStringVariable(void)
{
    TestOctetString_s testTable[] =
            {
              { { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }, 8, "8 Octet String" },
              { { 0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9, 0xF8 }, 8, "8 Octet String (Reverse)" },
              { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 }, 8, "8 Octet String (Zero Start)" },
              { { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }, 6, "6 Octet String" },
              { { }, 0, "Empty Octet String" }, // Edge case: empty string
              { { 0x01 }, 1, "Single Octet String" }, // Edge case: single octet
              { { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                  0x09,
                  0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                  0x11,
                  0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                  0x19,
                  0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
                  0x21,
                  0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
                  0x29,
                  0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
                  0x31,
                  0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
                  0x39,
                  0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40,
                  0x41,
                  0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
                  0x49,
                  0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
                  0x51,
                  0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
                  0x59,
                  0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60,
                  0x61,
                  0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
                  0x69,
                  0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
                  0x71,
                  0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
                  0x79,
                  0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80,
                  0x81,
                  0x82, 0x83 },
                131, "131 octets" }, // 131 octets

              { { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                  0x09,
                  0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                  0x11,
                  0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                  0x19,
                  0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
                  0x21,
                  0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
                  0x29,
                  0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
                  0x31,
                  0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
                  0x39,
                  0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40,
                  0x41,
                  0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
                  0x49,
                  0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
                  0x51,
                  0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
                  0x59,
                  0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60,
                  0x61,
                  0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
                  0x69,
                  0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
                  0x71,
                  0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
                  0x79,
                  0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80,
                  0x81,
                  0x82, 0x83 },
                127, "127 octets" }, // 127 octets

              { { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                  0x09,
                  0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                  0x11,
                  0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                  0x19,
                  0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
                  0x21,
                  0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
                  0x29,
                  0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
                  0x31,
                  0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
                  0x39,
                  0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40,
                  0x41,
                  0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
                  0x49,
                  0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
                  0x51,
                  0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
                  0x59,
                  0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60,
                  0x61,
                  0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
                  0x69,
                  0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
                  0x71,
                  0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
                  0x79,
                  0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80,
                  0x81,
                  0x82, 0x83 },
                128, "128 octets" }, // 128 octets
            };

    int test_count = sizeof(testTable) / sizeof(TestOctetString_s);
    int passed = 0;
    printf("\n=== Testing axdr_encode/decode_octetstring variable ===\n\n");

    for (int i = 0; i < test_count; i++)
    {
        printf("Test case %d: %s\n", i + 1, testTable[i].description);
        printf("  Input octets: ");

        // Print the octet string
        for (uint64_t j = 0; j < testTable[i].octet_count; j++)
        {
            printf("%02X ", testTable[i].octet[j]);
        }
        printf("(%ld octets)\n", testTable[i].octet_count);

        // Create encoder buffer
        AxdrBuffer *enc_buf = axdr_buffer_new_encoder(64);
        if (!enc_buf)
        {
            printf("Failed to create encoder buffer\n");
            continue;
        }

        printf("  Encoding octet string...\n");
        // Encode value
        bool encode_result = axdr_encode_octetstring_var(enc_buf, testTable[i].octet, testTable[i].octet_count);
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
        uint8_t *decoded_octets = NULL;
        int64_t decoded_octet_count = 0; // Clear output buffer
        bool decode_result = axdr_decode_octetstring_var(dec_buf, &decoded_octets, &decoded_octet_count);
        if (!decode_result)
        {
            printf("  FAILED: Decoding failed\n");
        }
        else
        {
            printf("  Decoded octets: ");
            for (uint64_t j = 0; j < decoded_octet_count; j++)
            {
                printf("%02X ", decoded_octets[j]);
            }
            printf("(%ld octets)\n", decoded_octet_count);

            // Compare results
            bool match = true;
            for (uint64_t j = 0; j < testTable[i].octet_count; j++)
            {
                if (decoded_octets[j] != testTable[i].octet[j])
                {
                    match = false;
                    break;
                }
            }

            if (!match)
            {
                printf("  FAILED: Decoded octets don't match input\n");
            }
            else
            {
                passed++;
                printf("  PASSED: Successfully encoded and decoded octet string\n");
            }
        }

        // Cleanup
        axdr_buffer_free(enc_buf);
        axdr_buffer_free(dec_buf);
    }

    printf("=== Test Summary ===\n");
    printf("Total tests: %d\n", test_count);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", test_count - passed);
}
