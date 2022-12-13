#include "basedef.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>

/**************************************************
 * 功能描述: 获得当前时间字符串
 * ------------------------------------------------
 * 输入参数: buffer [out]: 当前时间的字符串, 长度
 *          至少为20个字节, 用以存储格式为"2020-07-01 16:13:33"
 * 输出参数: buffer
 * ------------------------------------------------
 * 返回值: 无
 * ------------------------------------------------
 * 修改日志:
 *      1. 日期: 2020年7月19日
 创建函数
 **************************************************/
void get_local_time(char *buf, u32 bufLen)
{
    struct timeval systime;
    struct tm timeinfo;

    gettimeofday(&systime, NULL);
    localtime_r(&systime.tv_sec, &timeinfo);
    snprintf(buf, bufLen, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
            (timeinfo.tm_year + 1900), timeinfo.tm_mon + 1, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
            systime.tv_usec / 1000);
}

/**************************************************
 * 功能描述: 将字节数组, 以16进制格式, 打印到字符串out中, 以空格
 * 分开每个字节
 * ------------------------------------------------
 * 输入参数: buf [in]: 字节数组
 * 输入参数: len [in]: 字节数组长度
 * 输出参数: out, 输出的字符串
 * ------------------------------------------------
 * 返回值: 无
 * ------------------------------------------------
 * 修改日志:
 *      1. 日期: 2020年7月19日
 创建函数
 **************************************************/
void getBufString(char *buf, int len, char *out)
{
    int i = 0;
    char s[5] = { 0 };
    for (i = 0; i < len; i++)
    {
        sprintf(s, "%02X ", (unsigned char) buf[i]);
        strcat(out, s);
    }
}

/**************************************************
 * 功能描述: 将调试信息打印到文件描述符fp中
 * ------------------------------------------------
 * 输入参数: fp [in]: 文件描述符
 * 输入参数: file [in]: 调试信息所在的文件名
 * 输入参数: func [in]: 调试信息所在的函数名
 * 输入参数: line [in]: 调试信息所在的行号
 * 输出参数: 无
 * ------------------------------------------------
 * 返回值: 无
 * ------------------------------------------------
 * 修改日志:
 *      1. 日期: 2020年7月19日
 创建函数
 **************************************************/
void debugBufFormat2fp(FILE *fp, const char *file, const char *func, int line,
        char *buf, int len, const char *fmt, ...)
{
    va_list ap;
    char bufTime[25] = { 0 };

    if (fp != NULL)
    {
        get_local_time(bufTime, sizeof(bufTime));
        fprintf(fp, "[%s][%s][%s()][%d]: ", bufTime, file, func, line);
        va_start(ap, fmt);
        vfprintf(fp, fmt, ap);
        va_end(ap);

        if (buf != NULL && len > 0)
        {
            char *s = (char*) calloc(1, 3 * (len + 1)); //多开3个字符的余量
            if (s != NULL)
            {
                getBufString(buf, len, s);
                fprintf(fp, "%s", s);
                free(s);
            }
        }
        fprintf(fp, "\n");
        fflush(fp);
        if (fp != stdout && fp != stderr)
            fclose(fp);
    }
}

int myprintk(const char *fmt, ...)
{
    va_list args;
    int i;

    char buf[1024];
    va_start(args, fmt);
    i = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    printf("myprintk float: %s", buf);

    return i;
}

void printfloat(unsigned char *pFloat)
{
    if (pFloat == NULL)
    {
        return;
    }

    float *pf = (float*) pFloat;

    printf("float: %f\n", *pf);
    myprintk("float: %f\n", *pf);
}

void inverseArray(u8 *buf, int bufSize)
{
    u16 i = 0;
    u8 tmp = 0;
    for (i = 0; i < bufSize / 2; i++)
    {
        tmp = buf[i];
        buf[i] = buf[bufSize - i - 1];
        buf[bufSize - i - 1] = tmp;
    }
}


/**************************************************
 * 功能描述: 计算一个缓冲区的crc16值
 * ------------------------------------------------
 * 输入参数: buf [in]: 缓冲区
 * 输入参数: len [in]: 缓冲区长度
 * 输出参数: 无
 * ------------------------------------------------
 * 返回值: crc16值
 * ------------------------------------------------
 * 修改日志:
 *      1. 日期: 2020年7月19日
 创建函数
 **************************************************/
u16 calcCRC16(u8 *buf, u32 len)
{
    u16 crc = 0xFFFF;
    int pos = 0;
    int i = 0;

    for (pos = 0; pos < len; pos++)
    {
        crc ^= (u16) buf[pos];

        for (i = 8; i > 0; i--)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= POLY;
            } else
                crc >>= 1;
        }
    }

    return crc;
}

u32 modbusRtuToTcp(u8 *buf, u32 bufLen, u8 *outbuf, u32 outbufLen, u16 seq)
{
    if (bufLen <= 4 || outbufLen < (bufLen - 2 + 6))
    {
        return 0;
    }

    u8 *pbuf = outbuf;
    u32 outlen = 0;

    pbuf[0] = ((seq >> 8) & 0x00FF);
    pbuf[1] = (seq & 0xFF);
    pbuf += 2;
    outlen += 2;

    memset(pbuf, MODBUS_PROTOCOL, 2);
    pbuf += 2;
    outlen += 2;

    u16 datalen = bufLen - 2;       //减去子设备id和crc校验
    pbuf[0] = ((datalen >> 8) & 0x00FF);
    pbuf[1] = (datalen & 0xFF);
    pbuf += 2;
    outlen += 2;

    memcpy(pbuf, buf, datalen);
    outlen += datalen;

    return outlen;
}

u32 modbusTcpToRtu(u8 *buf, u32 bufLen, u8 *outbuf, u32 outbufLen)
{
    if (bufLen < 7)
    {
        return 0;
    }

    u8 *pbuf = outbuf;

    u16 datalen = ((buf[4] << 8) | buf[5]);

    if (outbufLen < (datalen + 2))
    {
        return 0;
    }

    memcpy(pbuf, buf + 6, datalen);

    u16 crc = calcCRC16(pbuf, datalen);

    memcpy(pbuf + datalen, &crc, sizeof(crc));

    return (datalen + sizeof(crc));
}

int StrtoBcd(const char *p, unsigned char *pbcd, INT32 len)
{
    UINT8 tmpValue;
    INT32 i;
    INT32 j;
    INT32 m;
    INT32 sLen;
    const char *pstr = NULL;

    if (*p == '-')
    {
        pstr = p + 1;
    } else
    {
        pstr = p;
    }

    sLen = strlen(pstr);

    for (i = 0; i < sLen; i++)
    {
        if ((pstr[i] < '0') || ((pstr[i] > '9') && (pstr[i] < 'A'))
                || ((pstr[i] > 'F') && (pstr[i] < 'a')) || (pstr[i] > 'f'))
        {
            sLen = i;
            break;
        }
    }

    sLen = (sLen <= (len * 2)) ? sLen : sLen * 2;
    memset((void*) pbcd, 0x00, 4);

    for (i = sLen - 1, j = 0, m = 0; (i >= 0) && (m < len); i--, j++)
    {
        if ((pstr[i] >= '0') && (pstr[i] <= '9'))
        {
            tmpValue = pstr[i] - '0';
        } else if ((pstr[i] >= 'A') && (pstr[i] <= 'F'))
        {
            tmpValue = pstr[i] - 'A' + 0x0A;
        } else if ((pstr[i] >= 'a') && (pstr[i] <= 'f'))
        {
            tmpValue = pstr[i] - 'a' + 0x0A;
        } else
        {
            tmpValue = 0;
        }

        if ((j % 2) == 0)
        {
            pbcd[m] = tmpValue;
        } else
        {
            pbcd[m++] |= (tmpValue << 4);
        }

        if ((tmpValue == 0) && (pstr[i] != '0'))
        {
            break;
        }
    }

    return 0;
}

void HextoStr(float val, int m, int n, char *tmp)
{
    int j;
    char format[100] = { 0 };
    float floatvalue = val;

    int intvalue = (int) floatvalue;
    float dec = floatvalue - (float) intvalue;
    int mod = 1;
    for (int i = 0; i < m; i++)
    {
        mod *= 10;
    }

    intvalue = (intvalue % mod);
    floatvalue = (float) intvalue + dec;
    DEBUG_TIME_LINE("mod: %d, val: %f", mod, floatvalue);
    snprintf(format, sizeof(format), "%%0%d.%df", m + n + 1, n);
    DEBUG_TIME_LINE("format: %s", format);

    DEBUG_TIME_LINE(format, floatvalue);

    sprintf(tmp, format, floatvalue);
    DEBUG_TIME_LINE("tmp: %s", tmp);

    for (int i = 0; i < 10; i++)
    {
        if (tmp[i] == '.')
        {
            for (j = i; j < i + n; j++)
            {
                tmp[j] = tmp[j + 1];
            }

            tmp[j] = '\0';
        }
    }
}

s8 bcd2int32s(u8 *bcd, u8 len, u8 order, u8 signOnHigh, s32 *dint)
{
    int i = 0;
    s32 sign = 1;

    if (bcd == NULL || dint == NULL)
    {
        return -1;
    }

    if (len <= 0)
    {
        return -2;
    }

    *dint = 0;
    if (order == 1)
    {
        if (signOnHigh == 1)
        {
            sign = ((bcd[len - 1] & 0x80) ? -1 : 1);
            bcd[len - 1] &= 0x7F;
        }

        for (i = len; i > 0; i--)
        {
            *dint = (*dint * 10) + ((bcd[i - 1] >> 4) & 0xf);
            *dint = (*dint * 10) + (bcd[i - 1] & 0xf);
        }

        *dint *= sign;
    } else if (order == 0)
    {
        if (signOnHigh == 1)
        {
            sign = ((bcd[0] & 0x80) ? -1 : 1);
            bcd[0] &= 0x7F;
        }

        for (i = 0; i < len; i++)
        {
            *dint = (*dint * 10) + ((bcd[i] >> 4) & 0xf);
            *dint = (*dint * 10) + (bcd[i] & 0xf);
        }

        *dint *= sign;
    } else
        return -3;

    return 0;
}

s8 bcd2float(u8 *bcd, u8 len, u8 order, u8 signOnHigh, u8 declen, float *dfloat)
{
    if (dfloat == NULL)
    {
        return -1;
    }

    s32 sint = 0;
    s8 res = bcd2int32s(bcd, len, order, signOnHigh, &sint);
    DEBUG_TIME_LINE("sint: %d", sint);

    if (res < 0)
    {
        return -1;
    }

    int i = 0;
    float dec = 1.0f;
    for (i = 0; i < declen; i++)
    {
        dec *= 10.0f;
    }

    *dfloat = (float) sint / dec;

    return 0;
}

s8 bcd2floatBlock(u8 *bcd, u8 lenPerItem, u8 itemCount, u8 order, u8 signOnHigh,
        u8 declen, float *dfloat)
{
    if (dfloat == NULL)
    {
        return -1;
    }

    int i = 0;
    s8 res = 0;

    for (i = 0; i < itemCount; i++)
    {
        res = bcd2float(bcd + lenPerItem * i, lenPerItem, order, signOnHigh,
                declen, dfloat + i);
        if (res < 0)
        {
            return -1;
        }
    }

    return 0;
}

s8 float2s32(float f, u8 declen, s32 *intval)
{
    if (NULL == intval)
    {
        return -1;
    }

    int i = 0;
    for (i = 0; i < declen; i++)
    {
        f *= 10.0f;
    }

    *intval = (int) f;

    return 0;
}

void s32toBcd(s32 intval, u8 *bcd, u8 bcdlen, u8 signOnHigh)
{
    s32 val = intval;
    int i = 0;
    u8 reminder = 0;
    if (intval < 0)
    {
        val *= -1;
    }

    for (i = 0; i < bcdlen; i++)
    {
        reminder = val % 10;
        if (i % 2)
        {
            bcd[i / 2] |= ((reminder << 4) & 0xF0);
        } else
        {
            bcd[i / 2] = reminder & 0x0F;
        }

        val /= 10;
    }

    if (signOnHigh == 1 && intval < 0)
    {
        bcd[bcdlen / 2] |= 0x80;
    }
}

s8 float2bcd(float f, u8 *bcd, u8 bcdlen, u8 declen, u8 signOnHigh, u8 inverse)
{
    s32 intval = 0;
    s8 res = float2s32(f, declen, &intval);
    if (res < 0)
    {
        return -1;
    }

    s32toBcd(intval, bcd, bcdlen, signOnHigh);

    if (inverse == 0)
    {
        inverseArray(bcd, bcdlen / 2 + (bcdlen % 2 ? 1 : 0));
    }

    return 0;
}
