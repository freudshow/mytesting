#include "dlt645.h"
#include <string.h>

/******************************************************
 * 函数功能: 判断报文是否为DLT/645-2007报文
 * 注   意: 本函数所说的前导符, 不一定是0xFE, 或者0xFD等字符, 而是
 *         指非开始符(0x68)以外的任何字符
 * ---------------------------------------------------
 * @param [in] - pbuf, 报文缓冲区
 * @param [in] - len, 报文缓冲区长度
 * @param [out] - frmLen, 如果找到645报文, 在此返回645报文长度
 * @param [out] - start, 如果找到起始符, 在此返回起始符位置,
 *                  而不管是否找到真正的645报文
 * ---------------------------------------------------
 * @return - 是DLT/645-2007报文, 返回前导符个数, 包括0个或多个;
 *           不是DLT/645-2007报文, 且找到了起始符, 返回-1;
 *           不是DLT/645-2007报文, 且没找到起始符, 返回-2;
 ******************************************************/
int dlt64507_is645Frame(u8 *pbuf, u16 len, u16 *frmLen, u16 *start)
{
    if (NULL == pbuf || len < DLT645_07_MIN_FRAME_LEN)
    {
        return -1;
    }

    u16 i = 0;
    u16 j = 0;
    u16 opLen = 0;
    u16 preCount = 0;
    u8 *pTmp = pbuf;

    if (frmLen != NULL)
    {
        *frmLen = 0;
    }

    if (start != NULL)
    {
        *start = 0;
    }

    while (pTmp[i] != DLT645_07_START_CODE && i < len)
    {
        i++;
        preCount++;
    }

    //如果没找到开始符, 则查找失败
    if (preCount == len)
    {
        return -2;
    }

    if (start != NULL)
    {
        *start = preCount;
    }

    pTmp += preCount;
    opLen = len - preCount;

    if (opLen < DLT645_07_MIN_FRAME_LEN)
    {
        return -1;
    }

    head645_s head;
    memcpy(&head, pTmp, DLT645_07_HEAD_LEN);

    if (head.start1 != DLT645_07_START_CODE || head.start2 != DLT645_07_START_CODE)
    {
        return -1;
    }

    //逻辑地址必须是压缩BCD码; 或者是通配符0xAA
    //当地址中出现0xAA时, 比这个0xAA字节高的地址,
    //必须都是0xAA
    for (i = 0; i < DLT645_07_ADDR_LEN; i++)
    {
        if (DLT645_07_IS_COMBCD(head.addr[i]))
        {
            continue;
        }

        if (head.addr[i] != DLT645_07_WILDCHAR_ADDR)
        {
            return -1;
        }

        for (j = i; j < DLT645_07_ADDR_LEN; j++)
        {
            if (head.addr[j] != DLT645_07_WILDCHAR_ADDR)
            {
                return -1;
            }
        }
    }

    opLen = head.len + DLT645_07_MIN_FRAME_LEN;

    //如果剩下的长度小于计算出来的报文长度, 则不是1个完整645帧
    if (opLen > (len - preCount))
    {
        return -1;
    }

    if (pTmp[opLen - 1] != DLT645_07_END_CODE)
    {
        return -1;
    }

    if (pTmp[opLen - 2] != chkSum(pTmp, opLen - 2))
    {
        return -1;
    }

    if (frmLen != NULL)
    {
        *frmLen = opLen;
    }

    return preCount;
}

/******************************************************
 * 函数功能: 去掉报文的前导符
 * ---------------------------------------------------
 * @param - pbuf, 报文缓冲区
 * @param - len, 报文缓冲区长度
 * @param - pLen, 经过操作后, 报文缓冲区最后的长度
 * ---------------------------------------------------
 * @return - 经过操作后, 报文的开始指针
 ******************************************************/
u8* dlt64507_removePrefix(u8 *pbuf, u16 len, u16 *pLen)
{
    if (NULL == pbuf || len <= 0 || NULL == pLen)
    {
        return NULL;
    }

    u16 prefix = 0;
    while ((pbuf[prefix] != DLT645_07_START_CODE) && (prefix < len))
    {
        prefix++;
    }

    *pLen = (len - prefix);

    return pbuf + prefix;
}

/******************************************************
 * 函数功能: 从1个645报文中获取逻辑地址
 * ---------------------------------------------------
 * @param - pbuf, 报文缓冲区
 * @param - len, 报文缓冲区长度
 * @param - pAddr, 逻辑地址指针
 * @param - inverse, 是否将逻辑地址按大段重排,
 *                   0 - 不重排
 *                   1 - 重排
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void dlt64507_getAddrFromBuf(u8 *pbuf, u16 len, u8 *pAddr, u8 inverse)
{
    u16 frmlen = 0;
    u8 *start = dlt64507_removePrefix(pbuf, len, &frmlen);

    memcpy(pAddr, &start[1], DLT645_07_ADDR_LEN);

    if (inverse == 1)
    {
        inverseArray(pAddr, DLT645_07_ADDR_LEN);
    }
}

/******************************************************
 * 函数功能: 组织dlt645报文
 * ---------------------------------------------------
 * @param - pDlt645, dlt645结构体指针
 * @param - buf, 报文缓冲区指针
 * @param - bufSize, 报文缓冲区的最大长度
 * ---------------------------------------------------
 * @return - 最终得到的报文长度
 ******************************************************/
u16 dlt64507_compose(dlt645_p pDlt645, u8 *buf, u16 bufSize)
{
    if (NULL == pDlt645 || NULL == buf || bufSize < DLT645_07_MIN_FRAME_LEN)
    {
        return 0;
    }

    u8 *p = buf;

    //头部
    memcpy(p, &pDlt645->head, sizeof(head645_s));
    p += sizeof(head645_s);

    //数据域
    add33(pDlt645->data, pDlt645->head.len);
    memcpy(p, pDlt645->data, pDlt645->head.len);
    p += pDlt645->head.len;

    //校验和(pBuf, pos);
    *p = chkSum(buf, (p - buf));
    p++;

    //结束符
    *p = DLT645_07_END_CODE;
    p++;

    return (p - buf);
}

/******************************************************
 * 函数功能: 从一个报文缓冲区中取出一帧DLT/645-2007报文
 * ---------------------------------------------------
 * @param [in] - buf, 报文缓冲区
 * @param [in] - bufSize, 报文缓冲区长度
 * @param [out] - pLen, 如果找到645报文, 在此返回645报文长度
 * ---------------------------------------------------
 * @return - 找到DLT/645-2007报文, 返回前导符个数, 包括0个或多个;
 *           没找到DLT/645-2007报文, 返回-1;
 ******************************************************/
int dlt64507_getOne645Frame(u8 *buf, u16 bufSize, u16 *pLen)
{
    if (buf == NULL || bufSize == 0 || pLen == NULL)
    {
        return -1;
    }

    u16 start = 0;
    int preCount = 0;
    int res = 0;
    u8 *p = buf;
    u16 bufLen = bufSize;
    while (bufLen > 0)
    {
        res = dlt64507_is645Frame(p, bufLen, pLen, &start);
        if (res >= 0)
        {
            return preCount + res;
        }
        else if (res == -2)
        {
            return -1;
        }
        else if (res == -1)
        {
            if (start < bufLen)
            {
                start += 1;
                preCount += start;
                p += start;
                bufLen -= start;
            }
            else
            {
                break;
            }
        }
    }

    return preCount;
}

int getLastDlt645Frame(u8 *buf, u16 bufSize, u16 *start, u16 *framelen)
{
    if (start == NULL || framelen == NULL)
    {
        return -1;
    }

    u16 minlen = DLT645_07_MIN_FRAME_LEN;
    u16 endpos = bufSize - 1;

    while (buf[endpos] != DLT645_07_END_CODE && (endpos + 1) >= minlen)
    {
        endpos--;
    }

    u16 startpos = 0;

    for (; (endpos + 1) >= minlen; endpos--)
    {
        for (startpos = endpos + 1 - minlen; startpos >= 0 && startpos < bufSize; startpos--)
        {
            if (dlt64507_is645Frame(&buf[startpos], endpos - startpos + 1, NULL, NULL) == 0)
            {
                *start = startpos;
                *framelen = endpos - startpos + 1;
                return 0;
            }
        }
    }

    return -1;
}

void test645(void)
{
    u8 buf[] = { 0x68, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x68, 0x91, 0x04, 0x34, 0x38, 0x33, 0x37, 0x50, 0x16 };
    u16 len = sizeof(buf);

    u16 start = 0;
    u16 framelen = 0;
    int res = getLastDlt645Frame(buf, len, &start, &framelen);
    printf("res: %d, start: %u, framelen: %u\n", res, start, framelen);
    if (res >= 0)
    {
        DEBUG_BUFF_FORMAT(&buf[start], framelen, "get frame: --->>> ");
    }
    else
    {
        DEBUG_TIME_LINE("no 645 frame found");
    }

    u8 buferror[] = { 0x01, 0x68, 0x91, 0x68, 0x06, 0x05, 0x16, 0x03, 0x02, 0x01, 0x68, 0x91, 0x04, 0x34, 0x38, 0x33, 0x37, 0x50, 0x33, 0x37, 0x50 };
    len = sizeof(buferror);
    res = getLastDlt645Frame(buferror, len, &start, &framelen);
    printf("res: %d, start: %u, framelen: %u\n", res, start, framelen);
    if (res >= 0)
    {
        DEBUG_BUFF_FORMAT(&buferror[start], framelen, "get frame: --->>> ");
    }
    else
    {
        DEBUG_TIME_LINE("no 645 frame found");
    }
}
