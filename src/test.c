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

int match(const char *str, const char *pattern)
{
    regex_t re;
    int err;

    err = regcomp(&re, pattern, REG_EXTENDED);
    if (err != 0)
    {
        char buf[100];
        regerror(err, &re, buf, sizeof(buf));
        printf("FAIL: %s\n", buf);
        return 0;
    }

    err = regexec(&re, str, 0, NULL, 0);
    regfree(&re);

    if (err)
        return 0;

    return 1;
}

#define CCO_PROXY_SELF_CMD          "ccoRouter"         //维护规约, 直接调用载波代理的自身功能
#define CCO_PROXY_UPDATE_START      "childUpdateStart"  //载波升级LTU时的命令分类参数

typedef struct childUpStartStruct {
#define CHILD_UPDATE_PROTOCOL_DLT645    0
#define CHILD_UPDATE_PROTOCOL_MODBUS    1
    int protocol;
    u8 logicAddr[6];
    char addr[32];
    char filename[512];
    char path[512];
    u32 fileLen;
    u32 frameLen;
    u8 modbusAddr;
} childUpStart_s;

int main(int argc, char **argv)
{
    childUpStart_s devUpdate = { 0 };
    char cmd[] = "ccoRouter childUpdateStart 202204190321 1 ltudev.bin 1 600";

    char *ptoken = NULL;
    char *sep = " ";
    int i = 0;
    for (ptoken = strtok(cmd, sep), i = 0; ptoken != NULL; ptoken = strtok(NULL, sep), i++)
    {
        DEBUG_TIME_LINE("ptoken: %s", ptoken);
        switch (i)
        {
            case 2:
            {
                strcpy(devUpdate.addr, ptoken);
                DEBUG_TIME_LINE("addr: %s", devUpdate.addr);
            }
                break;
            case 3:
            {
                char protocol[8] = { 0 };
                strcpy(protocol, ptoken);
                devUpdate.protocol = atoi(protocol);
                DEBUG_TIME_LINE("protocol: %d", devUpdate.protocol);
            }
                break;
            case 4:
            {
                strcpy(devUpdate.filename, ptoken);
                DEBUG_TIME_LINE("filename: %s", devUpdate.filename);
            }
                break;
            case 5:
            {
                char modbusaddr[8] = { 0 };
                strcpy(modbusaddr, ptoken);
                devUpdate.modbusAddr = atoi(modbusaddr);
                DEBUG_TIME_LINE("modbusAddr: %d", devUpdate.modbusAddr);
            }
                break;
            case 6:
            {
                char frameLen[8] = { 0 };
                strcpy(frameLen, ptoken);
                devUpdate.frameLen = atoi(frameLen);
                DEBUG_TIME_LINE("frameLen: %d", devUpdate.frameLen);
            }
                break;
            default:
                break;
        }
    }

    DEBUG_TIME_LINE("addr: %s, protocol: %d, filename: %s, modbusAddr: %d, frameLen: %d", devUpdate.addr, devUpdate.protocol, devUpdate.filename, devUpdate.modbusAddr, devUpdate.frameLen);

    return 0;
}
