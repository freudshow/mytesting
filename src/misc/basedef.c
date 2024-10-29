#include "basedef.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include <error.h>
#include <errno.h>

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
    snprintf(buf, bufLen, "%04d-%02d-%02d %02d:%02d:%02d.%03ld", (timeinfo.tm_year + 1900), timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, systime.tv_usec / 1000);
}

/**************************************************
 * 功能描述: 获得当前日志目录下, 日志后缀数值最大的号码. 例如, 日志目录
 * 为 /var/logs/, 日志文件名的前缀是"1376log.", 假如当前时刻目录
 * "/var/logs/"下有文件"1376log.1", "1376log.2",
 *  "1376log.3", 3个文件, 那么此函数返回整数3
 * ------------------------------------------------
 * 输入参数: fname [in]: 日志文件存放的目录名
 * 输出参数: 无
 * ------------------------------------------------
 * 返回值: 成功, 返回日志文件后缀数的最大值; 失败, 返回 -1
 * ------------------------------------------------
 * 修改日志:
 *      1. 日期: 2020年7月19日
 创建函数
 **************************************************/
int getMaxFileNo(const char *fname)
{
    int ret = 0;
    char cmd[100] = { 0 }; //调用shell命令
    char result[128] = { 0 };
    FILE *fstream = NULL;

    sprintf(cmd, "ls -r %s*|cut -c %d-99", fname, (int) strlen(fname) + 2); //最大两位数

    if (NULL == (fstream = popen(cmd, "r"))) { //打开管道
        fprintf(stdout, "execute command failed: %s", strerror(errno));
        return -1;
    }

    if (0 != fread(result, sizeof(char), sizeof(result), fstream)) { //读取执行结果
        int i = 0;
        while (result[i] != '\n') //读取第一个数字, 'ls -r' 命令默认降序排列
            i++;

        result[i] = 0; //字符串结束符
        ret = atoi(&result[0]);
    } else {
        ret = -1;
    }

    pclose(fstream);

    return ret;
}

/**************************************************
 * 功能描述: 检查当前时刻, 日志目录下的日志文件是否达到设定的最大文件数,
 * 如果已经达到了最大文件数, 则删除后缀值最大的文件, 且将其前序文件的后
 * 缀值依次增1. 例如, 当前设定的最大日志文件数为3, 且当前日志文件目录
 * 下的日志文件数已达到3个: log.0, log.1, log.2, 那么就将文件
 * "log.2"删掉, 依次将log.0, log.1的文件名修改为log.1,
 * log.2, 以保持日志记录的时间顺序
 * ------------------------------------------------
 * 输入参数: fname [in]: 日志文件名的前缀
 * 输入参数: logCount [in]: 设定的日志文件的最大数
 * 输出参数: 无
 * ------------------------------------------------
 * 返回值: 成功, 返回0; 失败, 返回 -1
 * ------------------------------------------------
 * 修改日志:
 *      1. 日期: 2020年7月19日
 创建函数
 **************************************************/
int mvFiles(const char *fname, int logCount)
{
    if ((access(fname, F_OK)) != 0) {
        return -1;
    }

    char name[100] = { 0 };
    int max = getMaxFileNo(fname);
    int i = 0;

    if (max < 0)
        return -1;

    if ((max + 1) == logCount) {
        sprintf(name, "%s.%d", fname, max);
        max -= 1;
        unlink(name);
        sync();
    }

    char cmd[200] = { 0 };
    for (i = max; i > 0; i--) {
        sprintf(cmd, "mv %s.%d %s.%d", fname, i, fname, i + 1);

        if (system(cmd) == -1) {
            printf("mvFiles err \r\n");
            return -1;
        }
    }

    if (logCount > 1) {
        sprintf(cmd, "mv %s %s.%d", fname, fname, i + 1);

        if (system(cmd) == -1) {
            printf("mvFiles err \r\n");
            return -1;
        }
    }

    return 0;
}

/**************************************************
 * 功能描述: 检查当前时刻, 日志目录下的日志文件是否达到设定的最大文件数,
 * 如果已经达到了最大文件数, 则删除后缀值最大的文件, 且将其前序文件的后
 * 缀值依次增1. 例如, 当前设定的最大日志文件数为3, 且当前日志文件目录
 * 下的日志文件数已达到3个: log.0, log.1, log.2, 那么就将文件
 * "log.2"删掉, 依次将log.0, log.1的文件名修改为log.1,
 * log.2, 以保持日志记录的时间顺序
 * ------------------------------------------------
 * 输入参数: fname [in]: 文件名, 绝对路径
 * 输入参数: logsize [in]: 日志文件的最大长度
 * 输入参数: logCount [in]: 日志文件的最大个数
 * 输出参数: 无
 * ------------------------------------------------
 * 返回值: 成功, 返回0; 失败, 返回 -1
 * ------------------------------------------------
 * 修改日志:
 *      1. 日期: 2020年7月19日
 创建函数
 **************************************************/
int logLimit(const char *fname, int logsize, int logCount)
{
    char cmd[128] = { 0 };
    struct stat fileInfo = { 0 };
    int ret = 0;

    int res = stat(fname, &fileInfo);
    if (res == -1) {
        sprintf(cmd, "rm -f %s", fname);

        if (system(cmd) == -1) {
            printf("mvFiles err \r\n");
            return -1;
        }
        return -1;
    }

    /**
     * 检查当前日志文件名的最大值(以logCount为参照)
     * 比如, logCount = 10, 如果当前文件名有'.9'
     * 结尾的, 那么就将'fname.9'删掉, 依次将'fname.8'复制
     * 为'fname9', 'fname.7'复制为'fname.8'...'fname'
     * 复制为'fname.1'; 如果当前文件没有达到最大值,
     * 则依次复制当前最大值的文件名, 往后累加一个.
     * 所以, 最关键的是获取当前最大的文件名.
     */
    if (fileInfo.st_size > logsize) {
        ret = mvFiles(fname, logCount);
    }

    return ret;
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
void debugBufFormat2fp(FILE *fp, const char *file, const char *func, int line, char *buf, int len, const char *fmt, ...)
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
            int i = 0;
            for (i = 0; i < len; i++)
            {
                fprintf(fp, "%02X ", (u8) buf[i]);
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
            }
            else
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
    }
    else
    {
        pstr = p;
    }

    sLen = strlen(pstr);

    for (i = 0; i < sLen; i++)
    {
        if ((pstr[i] < '0') || ((pstr[i] > '9') && (pstr[i] < 'A')) || ((pstr[i] > 'F') && (pstr[i] < 'a')) || (pstr[i] > 'f'))
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
        }
        else if ((pstr[i] >= 'A') && (pstr[i] <= 'F'))
        {
            tmpValue = pstr[i] - 'A' + 0x0A;
        }
        else if ((pstr[i] >= 'a') && (pstr[i] <= 'f'))
        {
            tmpValue = pstr[i] - 'a' + 0x0A;
        }
        else
        {
            tmpValue = 0;
        }

        if ((j % 2) == 0)
        {
            pbcd[m] = tmpValue;
        }
        else
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
    }
    else if (order == 0)
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
    }
    else
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

s8 bcd2floatBlock(u8 *bcd, u8 lenPerItem, u8 itemCount, u8 order, u8 signOnHigh, u8 declen, float *dfloat)
{
    if (dfloat == NULL)
    {
        return -1;
    }

    int i = 0;
    s8 res = 0;

    for (i = 0; i < itemCount; i++)
    {
        res = bcd2float(bcd + lenPerItem * i, lenPerItem, order, signOnHigh, declen, dfloat + i);
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
        }
        else
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

/**************************************************
 * 功能描述: 计算一个缓冲区的累加和
 * ------------------------------------------------
 * 输入参数: buf [in]: 缓冲区
 * 输入参数: bufSize [in]: 缓冲区长度
 * 输出参数: 无
 * ------------------------------------------------
 * 返回值: 累加和
 * ------------------------------------------------
 * 修改日志:
 * 		1. 日期: 2020年7月19日
 创建函数
 **************************************************/
u8 chkSum(u8 *buf, u16 bufSize)
{
    u16 i = 0;
    u8 sum = 0;

    for (i = 0, sum = 0; i < bufSize; i++)
        sum += buf[i];

    return sum;
}

/**************************************************
 * 功能描述: 将一个缓冲区的每个字节加0x33
 * ------------------------------------------------
 * 输入参数: buf [in]: 缓冲区
 * 输入参数: bufSize [in]: 缓冲区长度
 * 输出参数: 无
 * ------------------------------------------------
 * 返回值: 无
 * ------------------------------------------------
 * 修改日志:
 * 		1. 日期: 2020年7月19日
 创建函数
 **************************************************/
void add33(u8 *buf, int bufSize)
{
    u16 i = 0;

    for (i = 0; i < bufSize; i++)
        buf[i] += 0x33;
}

/**************************************************
 * 功能描述: 将一个缓冲区的每个字节减0x33
 * ------------------------------------------------
 * 输入参数: buf [in]: 缓冲区
 * 输入参数: bufSize [in]: 缓冲区长度
 * 输出参数: 无
 * ------------------------------------------------
 * 返回值: 无
 * ------------------------------------------------
 * 修改日志:
 * 		1. 日期: 2020年7月19日
 创建函数
 **************************************************/
void minus33(u8 *buf, int bufSize)
{
    u16 i = 0;

    for (i = 0; i < bufSize; i++)
        buf[i] -= 0x33;
}

/******************************************************
 * base64映射表
 ******************************************************/
static const char base64_code[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

/******************************************************
 * 函数功能: 获取一个base64编码中的字符在base64映射表中的索引值
 * ---------------------------------------------------
 * @param c[in] - 一个base64编码
 * ---------------------------------------------------
 * @return 查找失败 - 返回 65;
 *         查找成功 - 返回0~64;
 ******************************************************/
static u8 idx_of_base64(char c)
{
    if (c <= 'Z' && c >= 'A')
        return (c - 'A');
    if (c <= 'z' && c >= 'a')
        return (c - 'a' + 26); //a偏移了26个字母
    if (c <= '9' && c >= '0')
        return (c - '0' + 52); //0偏移了52个字母
    if (c == '+')
        return 62;
    if (c == '/')
        return 63;
    if (c == '=')
        return 64;

    return 65;
}

/******************************************************
 * 函数功能: 将base64编码过的字符串, 解码为相应的二进制串
 * ---------------------------------------------------
 * @param enStr[in] - base64编码过的字符串
 * @param enSize[in] - base64编码过的字符串
 * @param deStr[out] - 解码出的二进制串
 * ---------------------------------------------------
 * @return 解码失败 - 返回 -1;
 *         解码成功 - 返回 二进制串长度;
 ******************************************************/
int decode_base64(char *enStr, u32 enSize, u8 *deBuf)
{
    //base64编码过的字串必须为4的整数倍
    if (enSize < 4 || (enSize % 4) != 0)
    {
        return -1;
    }

    u8 b[4];
    int i;
    u8 *pBuf = deBuf;
    for (i = 0; i < enSize; i += 4)
    {
        b[0] = idx_of_base64(enStr[i]);
        b[1] = idx_of_base64(enStr[i + 1]);
        b[2] = idx_of_base64(enStr[i + 2]);
        b[3] = idx_of_base64(enStr[i + 3]);
        *pBuf++ = (b[0] << 2 | b[1] >> 4);
        if (b[2] < 64)
        {
            *pBuf++ = ((b[1] << 4) | (b[2] >> 2));
            if (b[3] < 64)
            {
                *pBuf++ = ((b[2] << 6) | b[3]);
            }
        }
    }

    return (int) (pBuf - deBuf);
}

/******************************************************
 * 函数功能: 计数base64编码当中符号'='出现的次数
 * ---------------------------------------------------
 * @param enStr[in] - base64编码过的字符串
 * @param enSize[in] - base64编码过的字符串
 * ---------------------------------------------------
 * @return 失败 - 返回 -1;
 *         成功 - 返回'='出现的次数;
 ******************************************************/
int cnt_of_pad(char *enStr, u32 enSize)
{
    if (enStr == NULL || enSize == 0)
    {
        return -1;
    }

    int cnt = 0;
    int i;
    for (i = 0; i < 2; i++)
    {
        if (enStr[enSize - 1 - i] == '=')
            cnt++;
    }

    return cnt;
}

/******************************************************
 * 函数功能: 将二进制串, 编码为base64字符串
 * ---------------------------------------------------
 * @param buf[in] - 二进制串
 * @param len[in] - 二进制串长度
 * @param enStr[out] - 编码后的字符串
 * ---------------------------------------------------
 * @return 失败 - 返回 -1;
 *         成功 - 返回编码后字符串的长度, 包括'\0';
 ******************************************************/
int encode_base64(u8 *buf, u32 len, char *enStr)
{
    if (buf == NULL || len == 0 || enStr == NULL)
    {
        return -1;
    }

    int idx;
    int i = 0;
    char *pStr = enStr;

    for (i = 0; i < len; i += 3)
    {
        idx = (buf[i] & 0xFC) >> 2;
        *pStr++ = base64_code[idx];

        idx = ((buf[i] & 0x03) << 4);
        if (i + 1 < len)
        {
            idx |= ((buf[i + 1] & 0xF0) >> 4);
            *pStr++ = base64_code[idx];
            idx = ((buf[i + 1] & 0x0F) << 2);
            if (i + 2 < len)
            {
                idx |= ((buf[i + 2] & 0xC0) >> 6);
                *pStr++ = base64_code[idx];
                idx = (buf[i + 2] & 0x3F);
                *pStr++ = base64_code[idx];
            }
            else
            {
                *pStr++ = base64_code[idx];
                *pStr++ = '=';
            }
        }
        else
        {
            *pStr++ = base64_code[idx];
            *pStr++ = '=';
            *pStr++ = '=';
        }
    }

    *pStr++ = '\0';

    return (int) (pStr - enStr);
}

