#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

// --- 1. 内存缓冲区和辅助函数 ---

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

// 初始化编码缓冲区
AxdrBuffer* axdr_buffer_new_encoder(size_t initial_size)
{
    AxdrBuffer *buf = malloc(sizeof(AxdrBuffer));
    if (!buf)
        return NULL;
    buf->data = malloc(initial_size);
    if (!buf->data)
    {
        free(buf);
        return NULL;
    }
    buf->size = initial_size;
    buf->pos = 0;
    buf->error = false;
    return buf;
}

// 初始化解码缓冲区
AxdrBuffer* axdr_buffer_new_decoder(uint8_t *data, size_t size)
{
    AxdrBuffer *buf = malloc(sizeof(AxdrBuffer));
    if (!buf)
        return NULL;
    buf->data = malloc(size); // Create a copy of the data
    if (!buf->data)
    {
        free(buf);
        return NULL;
    }
    memcpy(buf->data, data, size); // Copy the data
    buf->size = size;
    buf->pos = 0;
    buf->error = false;
    return buf;
}

// 释放缓冲区
void axdr_buffer_free(AxdrBuffer *buf)
{
    if (buf)
    {
        // 注意：解码器不拥有 data 指针
        if (buf->data)
        {
            free(buf->data);
            buf->data = NULL; // 防止重复释放
        }
        free(buf);
    }
}

// 确保编码缓冲区有足够的空间
static bool ensure_capacity(AxdrBuffer *buf, size_t needed)
{
    if (buf->error)
        return false;
    if (buf->pos + needed <= buf->size)
        return true;

    size_t new_size = buf->size * 2;
    while (new_size < buf->pos + needed)
        new_size *= 2;

    uint8_t *new_data = realloc(buf->data, new_size);
    if (!new_data)
    {
        buf->error = true;
        return false;
    }
    buf->data = new_data;
    buf->size = new_size;
    return true;
}

// --- 2. 基本类型编解码 ---

// Helper function to check if a value fits in the given number of bytes
static bool check_integer_range(int64_t value, int byte_size) {
    switch (byte_size) {
        case 1: return (value >= -128 && value <= 127);
        case 2: return (value >= -32768 && value <= 32767);
        case 3: return (value >= -8388608 && value <= 8388607);
        case 4: return (value >= -2147483648LL && value <= 2147483647LL);
        case 5: return (value >= -549755813888LL && value <= 549755813887LL);
        case 6: return (value >= -140737488355328LL && value <= 140737488355327LL);
        case 7: return (value >= -36028797018963968LL && value <= 36028797018963967LL);
        case 8: return true; // 8 bytes can hold any int64_t value
        default: return false;
    }
}

/*********************************************************************************************
 * 有符号整型编码 (固定长度)
 *********************************************************************************************
 * @param buf - 编码缓冲区
 * @param value - 要编码的整数值
 * @param byte_size - 指定整数的字节大小 (1-8)
 * @return true if successful, false if error occurs
 *********************************************************************************************/
bool axdr_encode_integer_fixed(AxdrBuffer *buf, int64_t value, int byte_size)
{
    if (buf->error || byte_size <= 0 || byte_size > 8)
        return false;

    // Check if the value fits in the specified number of bytes
    if (!check_integer_range(value, byte_size)) {
        buf->error = true;
        return false;
    }

    if (!ensure_capacity(buf, byte_size))
        return false;

    // Write bytes in big-endian order
    for (int i = byte_size - 1; i >= 0; i--) {
        buf->data[buf->pos++] = (value >> (i * 8)) & 0xFF;
    }
    return true;
}

/*********************************************************************************************
 * 有符号整型解码 (固定长度)
 *********************************************************************************************
 * @param buf - 解码缓冲区
 * @param value - 指向存储解码结果的整数指针
 * @param byte_size - 指定整数的字节大小 (1-8)
 * @return true if successful, false if error occurs
 *********************************************************************************************/
bool axdr_decode_integer_fixed(AxdrBuffer *buf, int64_t *value, int byte_size)
{
    if (buf->error || !value || byte_size <= 0 || byte_size > 8)
        return false;

    if (buf->pos + byte_size > buf->size) {
        buf->error = true;
        return false;
    }

    // Read bytes in big-endian order
    int64_t result = 0;
    uint8_t first_byte = buf->data[buf->pos];
    bool is_negative = (first_byte & 0x80) != 0;

    // Handle sign extension properly based on byte_size
    if (is_negative) {
        result = -1LL; // Start with all bits set for negative numbers
    }

    for (int i = 0; i < byte_size; i++) {
        result = (result << 8) | buf->data[buf->pos++];
    }

    // Verify the decoded value is within range for the given byte size
    if (!check_integer_range(result, byte_size)) {
        buf->error = true;
        return false;
    }

    *value = result;
    return true;
}

// 无符号整型编码 (固定长度)
bool axdr_encode_unsigned_fixed(AxdrBuffer *buf, uint64_t value, int byte_size)
{
    if (buf->error || byte_size <= 0 || byte_size > 8)
        return false;
    if (!ensure_capacity(buf, byte_size))
        return false;

    for (int i = byte_size - 1; i >= 0; i--)
    {
        buf->data[buf->pos++] = (value >> (i * 8)) & 0xFF;
    }
    return true;
}

// 无符号整型解码 (固定长度)
bool axdr_decode_unsigned_fixed(AxdrBuffer *buf, uint64_t *value, int byte_size)
{
    if (buf->error || !value || byte_size <= 0 || byte_size > 8)
        return false;
    if (buf->pos + byte_size > buf->size)
    {
        buf->error = true;
        return false;
    }

    *value = 0;
    for (int i = 0; i < byte_size; i++)
    {
        *value = (*value << 8) | buf->data[buf->pos++];
    }
    return true;
}

// 可变长度整型编码 (A-XDR规则)
bool axdr_encode_integer_var(AxdrBuffer *buf, int64_t value)
{
    if (buf->error)
        return false;

    // Case 1: Values 0-127 encoded directly in one byte
    if (value >= 0 && value <= 127) {
        if (!ensure_capacity(buf, 1))
            return false;
        buf->data[buf->pos++] = (uint8_t)value;
        return true;
    }

    // Case 2: Other values need length byte + value bytes
    // First determine minimum bytes needed for value in 2's complement
    uint8_t needed_bytes = 1;
    int64_t temp = (value < 0) ? ~value : value;
    
    while (temp > 127 || temp < -128) {
        needed_bytes++;
        temp >>= 8;
    }

    // Ensure we have enough space for length byte + value bytes
    if (!ensure_capacity(buf, needed_bytes + 1))
        return false;

    // Write length byte (MSB=1 to indicate length byte)
    buf->data[buf->pos++] = 0x80 | needed_bytes;

    // Write value bytes in big-endian order
    for (int i = needed_bytes - 1; i >= 0; i--) {
        buf->data[buf->pos++] = (value >> (i * 8)) & 0xFF;
    }

    return true;
}

// 可变长度整型解码 (A-XDR规则)
bool axdr_decode_integer_var(AxdrBuffer *buf, int64_t *value)
{
    if (buf->error || !value || buf->pos >= buf->size)
        return false;

    // Read first byte
    uint8_t first_byte = buf->data[buf->pos++];
    
    // Case 1: Values 0-127 are encoded directly
    if ((first_byte & 0x80) == 0) {
        *value = first_byte;
        return true;
    }
    
    // Case 2: First byte with MSB=1 indicates length
    uint8_t length = first_byte & 0x7F;
    if (length > 8 || buf->pos + length > buf->size) {
        buf->error = true;
        return false;
    }

    // Read the value bytes in big-endian order
    int64_t result = 0;
    for (int i = 0; i < length; i++) {
        result = (result << 8) | buf->data[buf->pos++];
    }

    // Handle sign extension if needed
    if (length < 8 && (result & (1LL << ((length * 8) - 1)))) {
        result |= ~((1LL << (length * 8)) - 1);
    }

    *value = result;
    return true;
}

// 布尔型编码
bool axdr_encode_boolean(AxdrBuffer *buf, bool value)
{
    if (buf->error)
        return false;
    if (!ensure_capacity(buf, 1))
        return false;
    buf->data[buf->pos++] = value ? 0xFF : 0x00; // 标准建议非零为真
    return true;
}

// 布尔型解码
bool axdr_decode_boolean(AxdrBuffer *buf, bool *value)
{
    if (buf->error || !value || buf->pos >= buf->size)
    {
        if (buf->pos >= buf->size)
            buf->error = true;
        return false;
    }
    *value = (buf->data[buf->pos++] != 0);
    return true;
}

// 枚举型编码 (作为整型编码)
bool axdr_encode_enumerated(AxdrBuffer *buf, int32_t value)
{
    // 通常枚举值较小，使用固定长度或可变长度整型编码
    // 这里假设使用固定2字节编码，符合DLMS常见实践
    return axdr_encode_integer_fixed(buf, value, 2);
}

// 枚举型解码
bool axdr_decode_enumerated(AxdrBuffer *buf, int32_t *value)
{
    int64_t tmp;
    bool res = axdr_decode_integer_fixed(buf, &tmp, 2);
    if (res && value)
        *value = (int32_t) tmp;
    return res;
}

/**********************************************************************************************
 * 编码固定长度的位串
 * ********************************************************************************************
 * @param buf - 编码缓冲区
 * @param bits - 指向位串数据的指针
 * @param bit_count - 位数
 * @return true if successful, false if error occurs
 *********************************************************************************************/
bool axdr_encode_bitstring_fixed(AxdrBuffer *buf, const uint8_t *bits, int bit_count)
{
    if (buf->error || !bits || bit_count < 0)
        return false;

    int byte_count = (bit_count + 7) / 8;  // Round up to nearest byte
    int unused_bits = (8 - (bit_count % 8)) % 8;

    if (!ensure_capacity(buf, byte_count + 1))
        return false;

    // Write bit string bytes
    if (byte_count > 0) {
        // Copy the bytes directly since we want MSB to LSB order
        memcpy(buf->data + buf->pos, bits, byte_count);

        // If we have a partial byte at the end, mask off unused bits
        // The bits should be right-aligned in the last byte
        if (bit_count % 8 != 0) {
            uint8_t mask = 0xFF << unused_bits;
            buf->data[buf->pos + byte_count - 1] &= mask;
        }

        buf->pos += byte_count;
    }

    return true;
}

/*********************************************************************************************
 * 位串解码 (固定长度)
 * *******************************************************************************************
 * @param buf - 解码缓冲区
 * @param bits - 指向存储解码结果的位串缓冲区
 * @param bit_count - 指向存储实际位数的整数指针
 * @param max_bits - bits 缓冲区的最大位数
 * @return true if successful, false if error occurs
 ********************************************************************************************/
bool axdr_decode_bitstring_fixed(AxdrBuffer *buf, uint8_t *bits, int *bit_count, int max_bits)
{
    if (buf->error || !bits || !bit_count || max_bits < 0 || buf->pos + 1 > buf->size)
        return false;

    // Read unused bits count
    int unused_bits = buf->data[buf->pos++];
    if (unused_bits > 7) {
        buf->error = true;
        return false;
    }

    // Calculate number of bytes needed
    int byte_count = (max_bits + 7) / 8;
    if (buf->pos + byte_count > buf->size) {
        buf->error = true;
        return false;
    }

    // Clear output buffer
    memset(bits, 0, (max_bits + 7) / 8);

    // Read the data
    if (byte_count > 0) {
        // Copy the bytes directly since we maintain MSB to LSB order
        memcpy(bits, buf->data + buf->pos, byte_count);
        
        // If we have a partial byte, ensure unused bits are cleared
        if (max_bits % 8 != 0) {
            uint8_t mask = 0xFF << unused_bits;
            bits[byte_count - 1] &= mask;
        }
        
        buf->pos += byte_count;
    }

    *bit_count = max_bits;
    return true;
}

// 位串编码 (可变长度)
bool axdr_encode_bitstring_var(AxdrBuffer *buf, const uint8_t *bits, int bit_count)
{
    if (buf->error || !bits || bit_count < 0)
        return false;
    int byte_count = (bit_count + 7) / 8;
    int unused_bits = (8 - (bit_count % 8)) % 8;
    if (!axdr_encode_integer_fixed(buf, bit_count, 4)) // 使用4字节编码长度
        return false;
    if (!ensure_capacity(buf, 1 + byte_count))
        return false;
    buf->data[buf->pos++] = unused_bits; // 第一个字节是未使用位数
    memcpy(buf->data + buf->pos, bits, byte_count);
    buf->pos += byte_count;
    return true;
}

// 字节串编码 (固定长度)
bool axdr_encode_octetstring_fixed(AxdrBuffer *buf, const uint8_t *octets, int octet_count)
{
    if (buf->error || !octets || octet_count < 0)
        return false;
    if (!ensure_capacity(buf, octet_count))
        return false;
    memcpy(buf->data + buf->pos, octets, octet_count);
    buf->pos += octet_count;
    return true;
}

// 字节串编码 (可变长度)
bool axdr_encode_octetstring_var(AxdrBuffer *buf, const uint8_t *octets, int octet_count)
{
    if (buf->error || !octets || octet_count < 0)
        return false;
    if (!axdr_encode_integer_var(buf, octet_count))
        return false;
    if (!ensure_capacity(buf, octet_count))
        return false;
    memcpy(buf->data + buf->pos, octets, octet_count);
    buf->pos += octet_count;
    return true;
}

// 字节串解码 (固定长度)
bool axdr_decode_octetstring_fixed(AxdrBuffer *buf, uint8_t *octets, int octet_count)
{
    if (buf->error || !octets || buf->pos + octet_count > buf->size)
    {
        if (buf->pos + octet_count > buf->size)
            buf->error = true;
        return false;
    }
    memcpy(octets, buf->data + buf->pos, octet_count);
    buf->pos += octet_count;
    return true;
}

// 字节串解码 (可变长度)
bool axdr_decode_octetstring_var(AxdrBuffer *buf, uint8_t **octets, int *octet_count)
{
    if (buf->error || !octets || !octet_count)
        return false;
    int64_t len;
    if (!axdr_decode_integer_var(buf, &len) || len < 0)
    {
        buf->error = true;
        return false;
    }
    if (buf->pos + len > buf->size)
    {
        buf->error = true;
        return false;
    }
    *octets = malloc(len);
    if (!(*octets) && len > 0)
    {
        buf->error = true;
        return false;
    }
    *octet_count = len;
    if (len > 0)
    {
        memcpy(*octets, buf->data + buf->pos, len);
        buf->pos += len;
    }
    return true;
}

// 可视串编码 (作为可变长度字节串)
bool axdr_encode_visiblestring(AxdrBuffer *buf, const char *str)
{
    if (!str)
        str = ""; // 处理 NULL 指针
    return axdr_encode_octetstring_var(buf, (const uint8_t*) str, strlen(str));
}

// 可视串解码 (作为可变长度字节串)
bool axdr_decode_visiblestring(AxdrBuffer *buf, char **str)
{
    uint8_t *tmp_str = NULL;
    int len;
    bool res = axdr_decode_octetstring_var(buf, &tmp_str, &len);
    if (res)
    {
        *str = malloc(len + 1);
        if (*str)
        {
            memcpy(*str, tmp_str, len);
            (*str)[len] = '\0';
        }
        else
        {
            buf->error = true;
        }
        free(tmp_str); // 释放临时分配的内存
    }
    return res && *str;
}

// 通用时间编码 (作为可变长度字节串)
bool axdr_encode_generalizedtime(AxdrBuffer *buf, time_t t)
{
    char time_str[32]; // 足够大
    struct tm *tm_info = gmtime(&t);
    if (!tm_info)
        return false;
    // 格式: YYYYMMDDHHMMSSZ (例如: 20231027103000Z)
    strftime(time_str, sizeof(time_str), "%Y%m%d%H%M%SZ", tm_info);
    return axdr_encode_visiblestring(buf, time_str);
}

// 通用时间解码 (作为可变长度字节串)
bool axdr_decode_generalizedtime(AxdrBuffer *buf, time_t *t)
{
    char *time_str = NULL;
    bool res = axdr_decode_visiblestring(buf, &time_str);
    if (res && t)
    {
        struct tm tm_info = { 0 };
        // 尝试解析 YYYYMMDDHHMMSSZ 格式
        if (sscanf(time_str, "%4d%2d%2d%2d%2d%2d", &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday, &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec) == 6)
        {
            tm_info.tm_year -= 1900;
            tm_info.tm_mon -= 1;
            tm_info.tm_isdst = 0; // UTC
            *t = timegm(&tm_info); // POSIX, 或使用 mktime 并调整时区
            if (*t == -1)
                res = false; // 解析失败
        }
        else
        {
            res = false; // 格式不匹配
        }
    }
    free(time_str);
    if (!res && t)
        *t = 0;
    return res;
}

// 空值编码 (作为显式标记类型处理)
bool axdr_encode_null(AxdrBuffer *buf)
{
    // A-XDR中NULL没有内容，但必须是标记类型。编码时只需标记。
    // 这里的实现假设标记已在调用者处处理（例如CHOICE）。
    // 如果需要单独编码，通常为空内容。
    return true; // 空操作
}

// 空值解码
bool axdr_decode_null(AxdrBuffer *buf)
{
    // 解码时只需确认标记，内容为空。
    return true; // 空操作
}

// --- 3. 复杂类型编解码 ---

// 标记类型编码 (显式)
bool axdr_encode_tag_explicit(AxdrBuffer *buf, int tag_class, int tag_number, void (*encoder)(AxdrBuffer*, void*), void *data)
{
    // A-XDR: 仅支持显式标记，且标记值需要编码
    // 这里简化处理，假设上下文相关类标记 [tag_number]
    if (tag_class != 0x80)
    { // 上下文相关类
        fprintf(stderr, "A-XDR: Only context-specific tags are supported in this simplified example.\n");
        buf->error = true;
        return false;
    }
    // 编码标记值 (简化为单字节)
    if (!ensure_capacity(buf, 1))
        return false;
    buf->data[buf->pos++] = (uint8_t) tag_number;

    // 编码内容
    encoder(buf, data);
    return !buf->error;
}

// 标记类型解码 (显式)
bool axdr_decode_tag_explicit(AxdrBuffer *buf, int expected_tag_number, void (*decoder)(AxdrBuffer*, void*), void *data)
{
    if (buf->pos >= buf->size)
    {
        buf->error = true;
        return false;
    }
    uint8_t tag = buf->data[buf->pos++];
    if (tag != expected_tag_number)
    {
        fprintf(stderr, "Tag mismatch: expected %d, got %d\n", expected_tag_number, tag);
        buf->error = true;
        return false;
    }
    decoder(buf, data);
    return !buf->error;
}

// 可选类型编码标记
bool axdr_encode_optional_tag(AxdrBuffer *buf, bool is_present)
{
    return axdr_encode_boolean(buf, is_present);
}

// 可选类型解码标记
bool axdr_decode_optional_tag(AxdrBuffer *buf, bool *is_present)
{
    return axdr_decode_boolean(buf, is_present);
}

// SEQUENCE OF 编码 (固定大小)
bool axdr_encode_sequence_of_fixed(AxdrBuffer *buf, void **elements, int count, void (*encoder)(AxdrBuffer*, void*))
{
    if (buf->error || count < 0)
        return false;
    for (int i = 0; i < count; i++)
    {
        encoder(buf, elements[i]);
        if (buf->error)
            return false;
    }
    return true;
}

// SEQUENCE OF 解码 (固定大小)
bool axdr_decode_sequence_of_fixed(AxdrBuffer *buf, void **elements, int count, void (*decoder)(AxdrBuffer*, void*))
{
    if (buf->error || count < 0)
        return false;
    for (int i = 0; i < count; i++)
    {
        decoder(buf, elements[i]);
        if (buf->error)
            return false;
    }
    return true;
}

// SEQUENCE OF 编码 (可变大小)
bool axdr_encode_sequence_of_var(AxdrBuffer *buf, void **elements, int count, void (*encoder)(AxdrBuffer*, void*))
{
    if (buf->error || count < 0)
        return false;
    if (!axdr_encode_integer_var(buf, count))
        return false;
    return axdr_encode_sequence_of_fixed(buf, elements, count, encoder);
}

// SEQUENCE OF 解码 (可变大小)
bool axdr_decode_sequence_of_var(AxdrBuffer *buf, void ***elements, int *count, void (*decoder)(AxdrBuffer*, void*))
{
    if (buf->error || !elements || !count)
        return false;
    int64_t len;
    if (!axdr_decode_integer_var(buf, &len) || len < 0)
    {
        buf->error = true;
        return false;
    }
    *count = (int) len;
    if (*count == 0)
    {
        *elements = NULL;
        return true;
    }
    *elements = calloc(*count, sizeof(void*));
    if (!(*elements))
    {
        buf->error = true;
        return false;
    }
    for (int i = 0; i < *count; i++)
    {
        // 注意：这里需要为每个元素分配内存，解码器需要知道如何做
        // 这是一个简化的例子，实际应用中可能需要更复杂的内存管理
        (*elements)[i] = malloc(100); // 示例大小，实际应根据类型确定
        if (!(*elements)[i])
        {
            // 清理已分配的内存
            for (int j = 0; j < i; j++)
                free((*elements)[j]);
            free(*elements);
            *elements = NULL;
            *count = 0;
            buf->error = true;
            return false;
        }
        decoder(buf, (*elements)[i]);
        if (buf->error)
        {
            // 清理
            for (int j = 0; j <= i; j++)
                free((*elements)[j]);
            free(*elements);
            *elements = NULL;
            *count = 0;
            return false;
        }
    }
    return true;
}

// --- 4. 测试用例 ---

// 测试结构体
typedef struct {
    int16_t int16_val;
    uint16_t uint16_val;
    bool bool_val;
    int32_t enum_val;
    uint8_t bitstring_val[2]; // 10 bits
    uint8_t octetstring_val[4];
    int octetstring_len;
    char *visiblestring_val;
    time_t time_val;
    int seq_of_count;
    int32_t *seq_of_vals;
} TestStruct;

void test_struct_encoder(AxdrBuffer *buf, void *data)
{
    TestStruct *ts = (TestStruct*) data;

    // 1. Integer16 (固定长度)
    axdr_encode_integer_fixed(buf, ts->int16_val, 2);

    // 2. Unsigned16 (固定长度)
    axdr_encode_unsigned_fixed(buf, ts->uint16_val, 2);

    // 3. Boolean
    axdr_encode_boolean(buf, ts->bool_val);

    // 4. Enumerated (固定长度)
    axdr_encode_enumerated(buf, ts->enum_val);

    // 5. BitString (固定长度, 例如 10 bits)
    axdr_encode_bitstring_fixed(buf, ts->bitstring_val, 10);

    // 6. OctetString (固定长度)
    axdr_encode_octetstring_fixed(buf, ts->octetstring_val, ts->octetstring_len);

    // 7. VisibleString (可变长度)
    axdr_encode_visiblestring(buf, ts->visiblestring_val);

    // 8. GeneralizedTime (可变长度)
    axdr_encode_generalizedtime(buf, ts->time_val);

    // 9. SEQUENCE OF (可变长度)
    axdr_encode_integer_var(buf, ts->seq_of_count); // 编码长度
    for (int i = 0; i < ts->seq_of_count; i++)
    {
        axdr_encode_integer_fixed(buf, ts->seq_of_vals[i], 4); // 假设每个是32位
    }
}

void test_struct_decoder(AxdrBuffer *buf, void *data)
{
    TestStruct *ts = (TestStruct*) data;

    int64_t tmp_int64;
    uint64_t tmp_uint64;
    
    // 1. Integer16
    if (!axdr_decode_integer_fixed(buf, &tmp_int64, 2))
        return;
    ts->int16_val = (int16_t) tmp_int64;

    // 2. Unsigned16
    if (!axdr_decode_unsigned_fixed(buf, &tmp_uint64, 2))
        return;
    ts->uint16_val = (uint16_t) tmp_uint64;

    // 3. Boolean
    if (!axdr_decode_boolean(buf, &ts->bool_val))
        return;

    // 4. Enumerated
    if (!axdr_decode_enumerated(buf, &ts->enum_val))
        return;

    // 5. BitString
    int bit_count;
    if (!axdr_decode_bitstring_fixed(buf, ts->bitstring_val, &bit_count, 16))
        return;

    // 6. OctetString
    if (!axdr_decode_octetstring_fixed(buf, ts->octetstring_val, ts->octetstring_len))
        return;

    // 7. VisibleString
    if (!axdr_decode_visiblestring(buf, &ts->visiblestring_val))
        return;

    // 8. GeneralizedTime
    if (!axdr_decode_generalizedtime(buf, &ts->time_val))
        return;

    // 9. SEQUENCE OF
    int64_t seq_len;
    if (!axdr_decode_integer_var(buf, &seq_len))
        return;
    
    if (seq_len < 0 || seq_len > 1000) // Add reasonable limit
    {
        buf->error = true;
        return;
    }

    ts->seq_of_count = (int)seq_len;
    ts->seq_of_vals = NULL;
    
    if (ts->seq_of_count > 0)
    {
        ts->seq_of_vals = malloc(ts->seq_of_count * sizeof(int32_t));
        if (!ts->seq_of_vals)
        {
            buf->error = true;
            return;
        }
        
        for (int i = 0; i < ts->seq_of_count; i++)
        {
            int64_t val;
            if (!axdr_decode_integer_fixed(buf, &val, 4))
            {
                free(ts->seq_of_vals);
                ts->seq_of_vals = NULL;
                ts->seq_of_count = 0;
                return;
            }
            ts->seq_of_vals[i] = (int32_t)val;
        }
    }
}

void print_test_struct(const TestStruct *ts)
{
    printf("TestStruct:\n");
    printf("  int16_val: %d\n", ts->int16_val);
    printf("  uint16_val: %u\n", ts->uint16_val);
    printf("  bool_val: %s\n", ts->bool_val ? "true" : "false");
    printf("  enum_val: %d\n", ts->enum_val);
    printf("  bitstring_val (2 bytes): 0x%02X 0x%02X\n", ts->bitstring_val[0], ts->bitstring_val[1]);
    printf("  octetstring_val (%d bytes): ", ts->octetstring_len);
    for (int i = 0; i < ts->octetstring_len; i++)
        printf("%02X ", ts->octetstring_val[i]);
    printf("\n");
    printf("  visiblestring_val: '%s'\n", ts->visiblestring_val ? ts->visiblestring_val : "NULL");
    printf("  time_val: %ld (%s)\n", ts->time_val, ctime(&ts->time_val));
    printf("  seq_of_vals (%d elements): ", ts->seq_of_count);
    for (int i = 0; i < ts->seq_of_count; i++)
        printf("%d, ", ts->seq_of_vals[i]);
    printf("\n");
}

void free_test_struct(TestStruct *ts)
{
    if (ts)
    {
        free(ts->visiblestring_val);
        free(ts->seq_of_vals);
    }
}

// Test bitstring encoding and decoding
void test_bitstring_codec(void)
{
    printf("\n--- Testing BitString Codec ---\n");

    // Test cases with different bit lengths
    struct {
        uint8_t bits[3];     // Input bits
        int bit_count;       // Number of bits to encode
        bool should_pass;    // Whether test should pass
        const char *desc;    // Test description
    } tests[] = {
        // 测试1：单个字节，8位全部使用 (10000001)
        {{0x81, 0x00, 0x00}, 8, true, "Single byte (10000001)"},
        
        // 测试2：单个位在最左侧 (10000000)
        {{0x80, 0x00, 0x00}, 1, true, "Single bit at MSB (1)"},
        
        // 测试3：12位跨两个字节 (101010101010)
        {{0xAA, 0xA0, 0x00}, 12, true, "12 bits across two bytes (101010101010)"},
        
        // 测试4：16位完整两个字节 (1010010110100101)
        {{0xA5, 0xA5, 0x00}, 16, true, "16 bits full two bytes (1010010110100101)"},
        
        // 测试5：24位三个字节 (111111110000000011111111)
        {{0xFF, 0x00, 0xFF}, 24, true, "24 bits three bytes (111111110000000011111111)"},
        
        // 测试6：无效位数
        {{0x00, 0x00, 0x00}, -1, false, "Invalid bit count"}
    };

    for (size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
        printf("\nTest Case %zu: %s\n", i + 1, tests[i].desc);
        
        // Create encoder buffer
        AxdrBuffer *enc_buf = axdr_buffer_new_encoder(32);
        if (!enc_buf) {
            printf("Failed to create encoder buffer\n");
            continue;
        }

        // Encode
        bool enc_result = axdr_encode_bitstring_fixed(enc_buf, tests[i].bits, tests[i].bit_count);
        printf("Encoding %s\n", enc_result ? "succeeded" : "failed");
        
        if (enc_result != tests[i].should_pass) {
            printf("Unexpected encoding result!\n");
            axdr_buffer_free(enc_buf);
            continue;
        }

        if (!enc_result) {
            axdr_buffer_free(enc_buf);
            continue;
        }

        // Print encoded data
        printf("Encoded data (%zu bytes): ", enc_buf->pos);
        for (size_t j = 0; j < enc_buf->pos; j++) {
            printf("%02X ", enc_buf->data[j]);
        }
        printf("\n");

        // Print bit pattern for verification
        if (tests[i].bit_count > 0) {
            printf("Bit pattern: ");
            for (int j = 0; j < tests[i].bit_count; j++) {
                int byte_index = j / 8;
                int bit_index = 7 - (j % 8);  // Start from MSB
                printf("%d", (tests[i].bits[byte_index] >> bit_index) & 0x01);
                if ((j + 1) % 8 == 0) printf(" ");
            }
            printf("\n");
        }

        // Create decoder buffer
        AxdrBuffer *dec_buf = axdr_buffer_new_decoder(enc_buf->data, enc_buf->pos);
        if (!dec_buf) {
            printf("Failed to create decoder buffer\n");
            axdr_buffer_free(enc_buf);
            continue;
        }

        // Decode
        uint8_t decoded_bits[3] = {0};
        int decoded_bit_count = 0;
        bool dec_result = axdr_decode_bitstring_fixed(dec_buf, decoded_bits, &decoded_bit_count, tests[i].bit_count);
        printf("Decoding %s\n", dec_result ? "succeeded" : "failed");

        if (dec_result) {
            // Print decoded bit pattern
            printf("Decoded pattern: ");
            for (int j = 0; j < decoded_bit_count; j++) {
                int byte_index = j / 8;
                int bit_index = 7 - (j % 8);  // Start from MSB
                printf("%d", (decoded_bits[byte_index] >> bit_index) & 0x01);
                if ((j + 1) % 8 == 0) printf(" ");
            }
            printf("\n");

            // Verify results
            bool match = true;
            if (decoded_bit_count != tests[i].bit_count) {
                match = false;
            } else {
                for (int j = 0; j < (decoded_bit_count + 7) / 8; j++) {
                    uint8_t mask = 0xFF;
                    if (j == (decoded_bit_count + 7) / 8 - 1) {
                        int remaining_bits = decoded_bit_count % 8;
                        if (remaining_bits != 0) {
                            mask = 0xFF << (8 - remaining_bits);
                        }
                    }
                    if ((decoded_bits[j] & mask) != (tests[i].bits[j] & mask)) {
                        match = false;
                        break;
                    }
                }
            }
            printf("Test %s\n", match ? "PASSED" : "FAILED");
        }

        axdr_buffer_free(enc_buf);
        axdr_buffer_free(dec_buf);
    }
}

void axdrtest(void)
{
    printf("--- A-XDR Codec Test ---\n");

    // Test bitstring codec first
    test_bitstring_codec();

    // 1. 准备测试数据
    TestStruct original_data = { .int16_val = -12345, .uint16_val = 54321, .bool_val = true, .enum_val = 2, .bitstring_val = { 0xAB, 0xCD },
            .octetstring_val = { 0x11, 0x22, 0x33, 0x44 }, .octetstring_len = 4, .visiblestring_val = strdup("Hello, A-XDR!"), .time_val = time(NULL),
            .seq_of_count = 3, .seq_of_vals = NULL
                    };
    original_data.seq_of_vals = malloc(3 * sizeof(int32_t));
    if (!original_data.seq_of_vals)
    {
        fprintf(stderr, "Failed to allocate memory for original_data.seq_of_vals\n");
        free_test_struct(&original_data);
        return;
    }

    original_data.seq_of_vals[0] = 100;
    original_data.seq_of_vals[1] = -200;
    original_data.seq_of_vals[2] = 300;

    printf("Original Data:\n");
    print_test_struct(&original_data);

    // 2. 编码
    AxdrBuffer *enc_buf = axdr_buffer_new_encoder(1024);
    if (!enc_buf)
    {
        fprintf(stderr, "Failed to create encoder buffer\n");
        free_test_struct(&original_data);
        return;
    }

    test_struct_encoder(enc_buf, &original_data);
    if (enc_buf->error)
    {
        fprintf(stderr, "Encoding failed\n");
        axdr_buffer_free(enc_buf);
        enc_buf = NULL;
        free_test_struct(&original_data);
        return;
    }

    printf("\nEncoded data (%zu bytes):\n", enc_buf->pos);
    for (size_t i = 0; i < enc_buf->pos; i++)
    {
        printf("%02X ", enc_buf->data[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    if (enc_buf->pos % 16 != 0)
        printf("\n");
    printf("Encoded size: %zu bytes\n", enc_buf->pos);

    // 3. 解码
    AxdrBuffer *dec_buf = axdr_buffer_new_decoder(enc_buf->data, enc_buf->pos);
    if (!dec_buf)
    {
        fprintf(stderr, "Failed to create decoder buffer\n");
        axdr_buffer_free(enc_buf);
        enc_buf = NULL;
        free_test_struct(&original_data);
        return;
    }

    TestStruct decoded_data = { 0 }; // 初始化为0
    decoded_data.octetstring_len = 4; // 解码器需要知道固定长度
    test_struct_decoder(dec_buf, &decoded_data);
    if (dec_buf->error)
    {
        fprintf(stderr, "Decoding failed\n");
        axdr_buffer_free(enc_buf);
        enc_buf = NULL;
        axdr_buffer_free(dec_buf);
        dec_buf = NULL;
        free_test_struct(&original_data);
        free_test_struct(&decoded_data); // 部分解码的数据可能需要清理
        return;
    }

    printf("\nDecoded Data:\n");
    print_test_struct(&decoded_data);

    // 4. 验证
    bool success = true;
    success &= (original_data.int16_val == decoded_data.int16_val);
    success &= (original_data.uint16_val == decoded_data.uint16_val);
    success &= (original_data.bool_val == decoded_data.bool_val);
    success &= (original_data.enum_val == decoded_data.enum_val);
    success &= (memcmp(original_data.bitstring_val, decoded_data.bitstring_val, 2) == 0);
    success &= (memcmp(original_data.octetstring_val, decoded_data.octetstring_val, original_data.octetstring_len) == 0);
    success &= (strcmp(original_data.visiblestring_val, decoded_data.visiblestring_val) == 0);
    // 时间比较可能有精度损失，这里简化处理
    success &= (abs(difftime(original_data.time_val, decoded_data.time_val)) < 2);
    success &= (original_data.seq_of_count == decoded_data.seq_of_count);
    if (original_data.seq_of_vals && decoded_data.seq_of_vals) // 检查指针是否为 NULL
    {
        for (int i = 0; i < original_data.seq_of_count; i++)
        {
            success &= (original_data.seq_of_vals[i] == decoded_data.seq_of_vals[i]);
        }
    }
    else
    {
        success = false;
    }

    if (success)
    {
        printf("\n--- Test PASSED ---\n");
    }
    else
    {
        printf("\n--- Test FAILED ---\n");
    }

    // 5. 清理
    axdr_buffer_free(enc_buf);
    enc_buf = NULL;
    axdr_buffer_free(dec_buf);
    dec_buf = NULL;
    free_test_struct(&original_data);
    free_test_struct(&decoded_data);
}
