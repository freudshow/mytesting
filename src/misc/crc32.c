#include "basedef.h"

#include <unistd.h>

#define IOT_POLY 0xEDB88320UL

u32 crc32(const void *data, u32 len)
{
    const u8 *bytes = (u8*) data;
    u32 crc = 0xFFFFFFFFUL;

    // 循环处理每个字节
    for (u32 i = 0; i < len; i++)
    {
        crc ^= bytes[i];        // 把当前字节与 crc 的低 8 位进行异或操作

        // 处理当前字节的 8 位，每次处理一位
        for (int j = 0; j < 8; j++)
        {
            if (crc & 1)
            {      // 如果 crc 的最低位为 1，则右移并与多项式除数进行异或操作
                crc = (crc >> 1) ^ IOT_POLY;
            }
            else
            {            // 否则，只右移一个比特位
                crc >>= 1;
            }
        }
    }

    return ~crc;                // 取反操作得到最终结果
}

u32 crc32File(const char *fullname)
{
    u8 buf[512] = { 0 };
    u32 crc = 0xFFFFFFFFUL;
    u32 offset = 0;
    u32 bytes = 0;
    u32 i = 0;

    int fd = open(fullname, O_RDONLY, 0777);
    if (fd < 0)
    {
        return 0xFFFFFFFF;
    }

    struct stat st = { 0 };
    if (stat(fullname, &st) != 0)
    {
        return 0xFFFFFFFF;
    }

    lseek(fd, 0, SEEK_SET);

    while ((bytes = read(fd, buf, sizeof(buf))) > 0)
    {
        for (u32 i = 0; i < bytes; i++)
        {
            crc ^= buf[i];        // 把当前字节与 crc 的低 8 位进行异或操作

            // 处理当前字节的 8 位，每次处理一位
            for (int j = 0; j < 8; j++)
            {
                if (crc & 1)        // 如果 crc 的最低位为 1，则右移并与多项式除数进行异或操作
                {
                    crc = (crc >> 1) ^ IOT_POLY;
                }
                else        // 否则，只右移一个比特位
                {
                    crc >>= 1;
                }
            }
        }

        offset += bytes;
        i++;
    }

    close(fd);

    return ~crc;
}
