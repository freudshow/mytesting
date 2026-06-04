#ifndef DLT_790_6_H
#define DLT_790_6_H
#include <stdint.h>
#include <stdbool.h>

#define AXDR_BUFFER_RECOMMEND_SIZE  8192

// 用于编码和解码的缓冲区结构
typedef struct AxdrBufferStruct {
    uint8_t *data;
    size_t size; //data的長度
    size_t pos; // 当前读写位置
    size_t deepth; // 当前嵌套深度
    bool error; // 标记操作是否出错
    bool isEncodeTag; // 标记是否正在编码类型标签
    bool isEncodeLength; // 标记是否正在编码长度
    struct AxdrBufferStruct *children; // 子缓冲区指针，用于嵌套结构
} AxdrBuffer;

#ifdef __cplusplus
extern "C" {
#endif

AxdrBuffer* axdr_buffer_new_encoder(size_t initial_size);
AxdrBuffer* axdr_buffer_new_decoder(uint8_t *data, size_t size);
void axdr_buffer_free(AxdrBuffer *buf);
bool ensure_capacity(AxdrBuffer *buf, size_t needed);
bool axdr_encode_integer_fixed(AxdrBuffer *buf, int64_t value, int byte_size);
bool axdr_decode_integer_fixed(AxdrBuffer *buf, int64_t *value, int byte_size);
bool axdr_encode_unsigned_fixed(AxdrBuffer *buf, uint64_t value, int byte_size);
bool axdr_decode_unsigned_fixed(AxdrBuffer *buf, uint64_t *value, int byte_size);
bool axdr_encode_integer_var(AxdrBuffer *buf, int64_t value);
bool axdr_decode_integer_var(AxdrBuffer *buf, int64_t *value);
bool axdr_encode_boolean(AxdrBuffer *buf, bool value);
bool axdr_decode_boolean(AxdrBuffer *buf, bool *value);
bool axdr_encode_enumerated(AxdrBuffer *buf, int32_t value);
bool axdr_decode_enumerated(AxdrBuffer *buf, int32_t *value);
bool axdr_encode_optional_tag(AxdrBuffer *buf, bool is_present);
bool axdr_encode_bitstring_fixed(AxdrBuffer *buf, const uint8_t *bits, int64_t bit_count);
bool axdr_decode_bitstring_fixed(AxdrBuffer *buf, uint8_t *bits, uint64_t *bit_count, int64_t bitLengh);
bool axdr_encode_bitstring_var(AxdrBuffer *buf, const uint8_t *bits, int64_t bit_count);
bool axdr_decode_bitstring_var(AxdrBuffer *buf, const uint8_t *bits, uint64_t bits_buffer_size, uint64_t *bit_count);
bool axdr_encode_octetstring_fixed(AxdrBuffer *buf, const uint8_t *octets, int64_t octet_count);
bool axdr_decode_octetstring_fixed(AxdrBuffer *buf, uint8_t *octets, int64_t octet_count);
bool axdr_encode_octetstring_var(AxdrBuffer *buf, const uint8_t *octets, int64_t octet_count);
bool axdr_decode_octetstring_var(AxdrBuffer *buf, uint8_t **octets, int64_t *octet_count);
bool axdr_encode_visiblestring(AxdrBuffer *buf, const char *str);
bool axdr_decode_visiblestring(AxdrBuffer *buf, char **str);
bool axdr_encode_generalizedtime(AxdrBuffer *buf, time_t t);
bool axdr_decode_generalizedtime(AxdrBuffer *buf, time_t *t);
bool axdr_encode_null(AxdrBuffer *buf);
bool axdr_decode_null(AxdrBuffer *buf);
bool axdr_encode_tag_explicit(AxdrBuffer *buf, int tag_class, int tag_number, void (*encoder)(AxdrBuffer*, void*), void *data);
bool axdr_decode_tag_explicit(AxdrBuffer *buf, int expected_tag_number, void (*decoder)(AxdrBuffer*, void*), void *data);
bool axdr_encode_optional_tag(AxdrBuffer *buf, bool is_present);
bool axdr_decode_optional_tag(AxdrBuffer *buf, bool *is_present);
bool axdr_encode_sequence_of_fixed(AxdrBuffer *buf, void **elements, int count, void (*encoder)(AxdrBuffer*, void*));
bool axdr_decode_sequence_of_fixed(AxdrBuffer *buf, void **elements, int count, void (*decoder)(AxdrBuffer*, void*));
bool axdr_encode_sequence_of_var(AxdrBuffer *buf, void **elements, int count, void (*encoder)(AxdrBuffer*, void*));
bool axdr_decode_sequence_of_var(AxdrBuffer *buf, void ***elements, int *count, void (*decoder)(AxdrBuffer*, void*));

#ifdef __cplusplus
}
#endif

#endif  // DLT_790_6_H
