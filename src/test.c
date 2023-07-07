/*
 ============================================================================
 Name        : test.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <regex.h>

#include "list.h"
#include "basedef.h"
#include "oled.h"
#include "tcp.h"
#include "sqlite3/sqlite3.h"
#include "ringbufpurec.h"

#define MIN_FRAME_LEN       5
#define MAX_FRAME_LEN       255

#define HEAD_LEN_0x10       1  // 固定帧头长度
#define HEAD_LEN_0x68       4  // 变化帧头长度
#define TAIL_LEN            2  // 帧尾长度

#define START_CODE_0x10     0x10  // 固定帧头
#define START_CODE_0x68     0x68  // 变化帧头
#define END_CODE            0x16  // 帧尾

typedef struct {
    u8 byStartCode;
    u8 byAsduInfo[254];
}ST_IEC101Frame_0x10;

typedef struct {
    u8 byStartCode;
    u8 byApduLen;
    u8 byApduLen2;
    u8 byStartCode2;
    u8 byAsduInfo[MAX_FRAME_LEN-HEAD_LEN_0x68];
}ST_IEC101Frame_0x68;

static pthread_mutex_t s_mtx = PTHREAD_MUTEX_INITIALIZER;

void* func1(void *arg)
{
    while (1)
    {
        int res = pthread_mutex_lock(&s_mtx);

        if (res == 0)
        {
            int count = 3;
            while (count)
            {
                DEBUG_TIME_LINE("res: %d", res);
                count--;
                sleep(1);
            }
        }
        else
        {
            DEBUG_TIME_LINE("res: %d, errno: %d, %s", res, errno, strerror(errno));
        }

        pthread_mutex_unlock(&s_mtx);
        DEBUG_TIME_LINE("unlocked");
    }
}

void* func2(void *arg)
{
    while (1)
    {
        int res = pthread_mutex_lock(&s_mtx);

        if (res == 0)
        {
            int count = 5;
            while (count)
            {
                DEBUG_TIME_LINE("res: %d", res);
                count--;
                sleep(1);
            }
        }
        else
        {
            DEBUG_TIME_LINE("res: %d, errno: %d, %s", res, errno, strerror(errno));
        }

        pthread_mutex_unlock(&s_mtx);
        DEBUG_TIME_LINE("unlocked");
    }
}

void creatThread(void)
{
    pthread_t id1;

    int res = pthread_create(&id1, NULL, func1, NULL);
    if (res != 0)
    {
        DEBUG_TIME_LINE("func1 pthread_create failed");
        return;
    }

    pthread_t id2;

    res = pthread_create(&id2, NULL, func2, NULL);
    if (res != 0)
    {
        DEBUG_TIME_LINE("func2 pthread_create failed");
        return;
    }
}

int findOneFrame(u8 *buf, u16 bufLen, u16 *start, u16 *frameLen)
{
    u16 idx = 0;
    u16 consumed = 0;
    u16 startPos = 0;
    u8 *p = buf;

    //找第一个 0x68
    while ((idx < bufLen) && (buf[idx] != 0x68))
    {
        idx++;
    }

    if (idx == bufLen)
    {
        return -1;
    }

    startPos = idx;
    p += idx;

    if (start != NULL)
    {
        *start = startPos;
    }

    consumed += idx;
    u16 len = 0;

    if (bufLen - consumed < 5)
    {
        return 0;
    }

    if (bufLen - consumed < sizeof(len))
    {
        return 0;
    }

    memcpy(&len, &p[1], sizeof(len));
    if (consumed + len > bufLen)
    {
        return 0;
    }

    if (p[len - 1] != 0x16)
    {
        return 0;
    }

    u8 cs = chkSum(&p[3], len - 5);
    if (cs != p[len - 2])
    {
        return 0;
    }

    if (frameLen != NULL)
    {
        *frameLen = len;
    }

    return (startPos + len);
}

int readFrame(char *str, u32 maxLen, u8 *buf)
{
    int state = 0; //0, 初始状态; 1, 空格状态; 2, 字节高状态; 3, 字节低状态; 4, 错误状态.
    u8 high = 0;
    u8 low = 0;
    u32 destLen = 0; //已扫描过的字节个数
    char *p = str;

    if (buf == NULL || str == NULL)
    {
        return -1;
    }

    while (*p != '\0')
    {
        if (!(isHex(*p) || isDelim(*p)))
        {
            state = 4;
            goto final;
        }

        switch (state)
        {
            case 0: //init state
                if (isDelim(*p))
                {
                    state = 1;
                }
                else if (isHex(*p))
                {
                    high = ASCII_TO_HEX(*p);
                    state = 2;
                }

                break;
            case 1: //space state
                if (isHex(*p))
                {
                    high = ASCII_TO_HEX(*p);
                    state = 2;
                }

                break;
            case 2: //high state
                if (isDelim(*p))
                {
                    state = 4;
                    goto final;
                }

                if (destLen < maxLen && isHex(*p))
                {
                    low = ASCII_TO_HEX(*p);

                    buf[destLen++] = (high << 4 | low);
                    high = low = 0;
                    state = 3;
                }
                else
                {
                    DEBUG_TIME_LINE("buf over flow");
                    goto final;
                }

                break;
            case 3: //low state
                if (isHex(*p))
                {
                    high = ASCII_TO_HEX(*p);
                    state = 2;
                }
                else if (isDelim(*p))
                {
                    state = 1;
                }
                break;
            default:
                goto final;
        }

        p++;
    }

final:
    //高位状态和非法状态均为不可接收状态
    if (state == 4 || state == 2)
    {
        DEBUG_TIME_LINE("存在非法字符, 或字符串格式非法\n");
        if (destLen > 0)
        {
            memset(buf, 0, destLen);
            destLen = 0;
        }

        return -1;
    }

    return destLen;
}


int SearchFrameFor101(u8* frmData,int dataLen,int linkLen,int* frmNullStart)
{
    int i = 0;
    u8 isFindStart = 0,frmLen = 0;
    u8 byEndCode = 0; //byFrmSum = 0;
    //查找帧头
    for (i = 0; i < dataLen; i++)
    {
        if ((frmData[i] != START_CODE_0x10) && (frmData[i] != START_CODE_0x68))
        {
            *frmNullStart = i + 1;
        }
        else
        {
            isFindStart = 1;
            break;
        }
    }

    if(isFindStart == 0)
        return 0;

    //帧长度判断
    if(frmData[i] == START_CODE_0x10) //固定帧长度
    {

        ST_IEC101Frame_0x10 *m_pRecvFrame_0x10 = (ST_IEC101Frame_0x10 *)(frmData + i);

        //根据链路地址长度确定帧长度
        frmLen = 4 + linkLen;
//        if (linkLen == 1)
//        {
//            frmLen = 5;
//        }
//        else
//        {
//            frmLen = 6;
//        }

        if (dataLen < (frmLen + *frmNullStart))//帧没有收全
        {
            return 0;
        }

//        byFrmSum  = m_pRecvFrame_0x10->byAsduInfo[frmLen - HEAD_LEN_0x10 - TAIL_LEN + 0];
        byEndCode = m_pRecvFrame_0x10->byAsduInfo[frmLen - HEAD_LEN_0x10 - TAIL_LEN + 1];

        if (byEndCode != END_CODE)
        {
            *frmNullStart += 1;
            return -1;
        }

//        byCalSum = CalSumByte((m_pRecvFrame_0x10->byAsduInfo), (wFrameLen - HEAD_LEN_0x10 - TAIL_LEN));

//        if (byFrmSum != byCalSum)
//        {
//            printf("\n\033[31m %s-%d : Error ! byFrmSum = %02X, byCalSum = %02X !\033[0m\n", __FUNCTION__, __LINE__, byFrmSum, byCalSum);

//            m_RecvBuf.wReadPtr += 1;
//            return (SF_FALSE);
//        }

        return (frmLen);

    }
    else  if(frmData[i] == START_CODE_0x68) //可变帧帧长度
    {
        ST_IEC101Frame_0x68 *m_pRecvFrame_0x68 = (ST_IEC101Frame_0x68 *)(frmData + i);

        if (m_pRecvFrame_0x68->byStartCode2 != START_CODE_0x68)
        {
            *frmNullStart += 1;
            return -1;//出现坏帧
        }

        if (m_pRecvFrame_0x68->byApduLen != m_pRecvFrame_0x68->byApduLen2)
        {
           *frmNullStart += 1;
           return -1;
        }

        frmLen = m_pRecvFrame_0x68->byApduLen + HEAD_LEN_0x68 + TAIL_LEN;

        if (dataLen < (frmLen + *frmNullStart))//帧没有收全
        {
            return 0;
        }


       byEndCode = m_pRecvFrame_0x68->byAsduInfo[frmLen - 4 - 2 + 1];

       if (byEndCode != 0x16)//结束符
       {
           *frmNullStart += 1;
           return -1;
       }

        return (frmLen);

    }

    return 0;
}

int main(int argc, char **argv)
{
    u8 frame[] = {0x10 ,0x8B ,0x01 ,0x00 ,0x8C ,0x16};
    int unvalidLen = 0;
    int oneFrameLen = SearchFrameFor101(frame, sizeof(frame), 2, &unvalidLen);
    return 0;
}
