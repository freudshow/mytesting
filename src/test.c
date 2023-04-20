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

findOneFrame(u8 *buf, u16 bufLen, u16 *start, u16 *frameLen)
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

int main(int argc, char **argv)
{
//    u8 buf[] = { 0x04, 0x00, 0x5F, 0x00, 0x00, 0x68, 0x2D, 0x00, 0x43, 0x04, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x05, 0x04, 0x00, 0x00, 0x10, 0x68, 0x20, 0x03, 0x19, 0x04, 0x22, 0x20, 0x68, 0x11, 0x04, 0x33, 0x34, 0x34, 0x35, 0x37, 0x16, 0x3E, 0x16 };
    u8 buf[] = { 0x04 };
    char str[8192] = { 0 };
    int len = encode_base64(buf, sizeof(buf), str);
    DEBUG_TIME_LINE("str: %s, len: %d", str, len);

    char enstr[] = "aC0AQwQAXwAAAGZVRDMiEZmZmZmZmQUEAAAQaCADGQQiIGgRBDM0NDU3Fj4W";
    u8 debuf[8192] = { 0 };
    int buflen = decode_base64(enstr, strlen(enstr), debuf);
    DEBUG_BUFF_FORMAT(debuf, buflen, "buf-{%d}: ", buflen);

    return 0;
}
