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

int main(int argc, char **argv)
{
    ringBuf_s ring;
    ringBuf_init(&ring, 10);

    u8 buf[50] = { 0 };
    u8 temp[512] = { 0 };
    int i = 0;
    for (i = 0; i < sizeof(buf); i++)
    {
        buf[i] = i;
    }

    DEBUG_TIME_LINE("PUSH 8:");
    ringBuf_pushData(&ring, buf, 8);
    ringBuf_printf(&ring);

    DEBUG_TIME_LINE("POP 7:");
    ringBuf_popData(&ring, temp, 7);
    ringBuf_printf(&ring);

    DEBUG_TIME_LINE("PUSH 9:");
    ringBuf_pushData(&ring, buf, 9);
    ringBuf_printf(&ring);

    DEBUG_TIME_LINE("EXTEND:");
    ringBuf_extendCap(&ring, 20);
    ringBuf_printf(&ring);

    return 0;
}
