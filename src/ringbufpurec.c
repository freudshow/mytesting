#include "ringbufpurec.h"
#include "malloc.h"

#include <stdio.h>

/******************************************************
 * 函数功能: 初始化环形队列
 * ---------------------------------------------------
 * @param - pRing, 环形队列指针
 * @param - capacity, 环形队列总容量
 * ---------------------------------------------------
 * @return - 成功, 返回RING_BUF_PURE_C_SUCCESS;
 *           参数不合法, 返回RING_BUF_PURE_C_PARAM_ERROR;
 *           发生错误, 返回RING_BUF_PURE_C_FATAL_ERROR.
 ******************************************************/
int ringBuf_init(ringBuf_p pRing, int capacity)
{
    if (NULL == pRing || capacity <= 0)
    {
        return RING_BUF_PURE_C_PARAM_ERROR;
    }

    pthread_mutex_init(&pRing->m, NULL);
    pRing->wPos = 0;
    pRing->rPos = 0;
    pRing->curSize = 0;
    pRing->pbuf = malloc(capacity);

    if (NULL == pRing->pbuf)
    {
        pRing->capacity = 0;
        return RING_BUF_PURE_C_FATAL_ERROR;
    }

    pRing->capacity = capacity;

    return RING_BUF_PURE_C_SUCCESS;
}

/******************************************************
 * 函数功能: 获取环形队列的总容量
 * ---------------------------------------------------
 * @param - pRing, 环形队列指针
 * ---------------------------------------------------
 * @return - 成功, 返回环形队列的总容量;
 *           参数不合法, 返回RING_BUF_PURE_C_PARAM_ERROR.
 ******************************************************/
int ringBuf_getCapacity(ringBuf_p pRing)
{
    if (NULL == pRing)
    {
        return RING_BUF_PURE_C_PARAM_ERROR;
    }

    return pRing->capacity;
}

/******************************************************
 * 函数功能: 判断环形队列是否为空
 * ---------------------------------------------------
 * @param - pRing, 环形队列指针
 * ---------------------------------------------------
 * @return - 参数不合法, 返回RING_BUF_PURE_C_PARAM_ERROR;
 *           空, 返回RING_BUF_PURE_C_TRUE;
 *           非空, 返回RING_BUF_PURE_C_FALSE.
 ******************************************************/
int ringBuf_isEmpty(ringBuf_p pRing)
{
    if (NULL == pRing)
    {
        return RING_BUF_PURE_C_PARAM_ERROR;
    }

    if (0 == pRing->curSize)
    {
        return RING_BUF_PURE_C_TRUE;
    }

    return RING_BUF_PURE_C_FALSE;
}

/******************************************************
 * 函数功能: 判断环形队列是否已满
 * ---------------------------------------------------
 * @param - pRing, 环形队列指针
 * ---------------------------------------------------
 * @return - 参数不合法, 返回RING_BUF_PURE_C_PARAM_ERROR;
 *           已满, 返回RING_BUF_PURE_C_TRUE;
 *           不满, 返回RING_BUF_PURE_C_FALSE.
 ******************************************************/
int ringBuf_isFull(ringBuf_p pRing)
{
    if (NULL == pRing)
    {
        return RING_BUF_PURE_C_PARAM_ERROR;
    }

    if (pRing->curSize == pRing->capacity)
    {
        return RING_BUF_PURE_C_TRUE;
    }

    return RING_BUF_PURE_C_FALSE;
}

/******************************************************
 * 函数功能: 获取环形队列的空闲长度
 * ---------------------------------------------------
 * @param - pRing, 环形队列指针
 * ---------------------------------------------------
 * @return - 成功, 返回环形队列的空闲长度;
 *           参数不合法, 返回RING_BUF_PURE_C_PARAM_ERROR.
 ******************************************************/
int ringBuf_getIdle(ringBuf_p pRing)
{
    if (NULL == pRing)
    {
        return RING_BUF_PURE_C_PARAM_ERROR;
    }

    return (pRing->capacity - pRing->curSize);
}

/******************************************************
 * 函数功能: 获取环形队列的元素个数
 * ---------------------------------------------------
 * @param - pRing, 环形队列指针
 * ---------------------------------------------------
 * @return - 成功, 返回环形队列的元素个数;
 *           参数不合法, 返回RING_BUF_PURE_C_PARAM_ERROR.
 ******************************************************/
int ringBuf_getSize(ringBuf_p pRing)
{
    if (NULL == pRing)
    {
        return RING_BUF_PURE_C_PARAM_ERROR;
    }

    return pRing->curSize;
}

/******************************************************
 * 函数功能: 读取环形队列的元素, 但不移动读指针
 * ---------------------------------------------------
 * @param - pRing, 环形队列指针
 * @param - buf, 读出缓冲区
 * @param - len, 读出缓冲区长度
 * ---------------------------------------------------
 * @return - 成功, 返回读出的元素个数;
 *           参数不合法, 返回RING_BUF_PURE_C_PARAM_ERROR.
 ******************************************************/
int ringBuf_readData(ringBuf_p pRing, u8 *buf, int len)
{
    if (NULL == pRing || NULL == buf || len <= 0)
    {
        return RING_BUF_PURE_C_PARAM_ERROR;
    }

    pthread_mutex_lock(&pRing->m);

    int actualLen = RING_BUF_PURE_C_MIN(pRing->curSize, len);
    int i = 0;
    int pos = pRing->rPos;

    for (i = 0, pos = pRing->rPos; i < actualLen; i++)
    {
        buf[i] = pRing->pbuf[pos];
        RING_BUF_PURE_C_MOVE_NEXT_POS(pos, pRing->capacity);
    }

    pthread_mutex_unlock(&pRing->m);

    return actualLen;
}

/******************************************************
 * 函数功能: 向环形队列写入元素
 * ---------------------------------------------------
 * @param - pRing, 环形队列指针
 * @param - buf, 写入缓冲区
 * @param - len, 写入缓冲区长度
 * ---------------------------------------------------
 * @return - 成功, 返回写入的元素个数;
 *           参数不合法, 返回RING_BUF_PURE_C_PARAM_ERROR.
 ******************************************************/
int ringBuf_pushData(ringBuf_p pRing, u8 *buf, int len)
{
    if (NULL == pRing || NULL == buf || len <= 0)
    {
        return RING_BUF_PURE_C_PARAM_ERROR;
    }

    pthread_mutex_lock(&pRing->m);

    int actualLen = RING_BUF_PURE_C_MIN(ringBuf_getIdle(pRing), len);
    int i = 0;

    for (i = 0; i < actualLen; i++)
    {
        pRing->pbuf[pRing->wPos] = buf[i];
        RING_BUF_PURE_C_MOVE_NEXT_POS(pRing->wPos, pRing->capacity);
    }

    pRing->curSize += actualLen;

    pthread_mutex_unlock(&pRing->m);

    return actualLen;
}

/******************************************************
 * 函数功能: 读取环形队列的元素, 同时移动读指针
 * ---------------------------------------------------
 * @param - pRing, 环形队列指针
 * @param - buf, 读出缓冲区
 * @param - len, 读出缓冲区长度
 * ---------------------------------------------------
 * @return - 成功, 返回读出的元素个数;
 *           参数不合法, 返回RING_BUF_PURE_C_PARAM_ERROR.
 ******************************************************/
int ringBuf_popData(ringBuf_p pRing, u8 *buf, int len)
{
    if (NULL == pRing || NULL == buf || len <= 0)
    {
        return RING_BUF_PURE_C_PARAM_ERROR;
    }

    pthread_mutex_lock(&pRing->m);

    int actualLen = RING_BUF_PURE_C_MIN(pRing->curSize, len);
    int i = 0;

    for (i = 0; i < actualLen; i++)
    {
        buf[i] = pRing->pbuf[pRing->rPos];
        RING_BUF_PURE_C_MOVE_NEXT_POS(pRing->rPos, pRing->capacity);
    }

    pRing->curSize -= actualLen;

    pthread_mutex_unlock(&pRing->m);

    return actualLen;
}

/******************************************************
 * 函数功能: 清空环形队列
 * ---------------------------------------------------
 * @param - pRing, 环形队列指针
 * ---------------------------------------------------
 * @return - 成功, RING_BUF_PURE_C_SUCCESS;
 *           参数不合法, 返回RING_BUF_PURE_C_PARAM_ERROR.
 ******************************************************/
int ringBuf_clear(ringBuf_p pRing)
{
    if (NULL == pRing)
    {
        return RING_BUF_PURE_C_PARAM_ERROR;
    }

    pthread_mutex_lock(&pRing->m);

    pRing->capacity = 0;
    pRing->wPos = 0;
    pRing->rPos = 0;
    pRing->curSize = 0;

    if (pRing->pbuf != NULL)
    {
        free(pRing->pbuf);
        pRing->pbuf = NULL;
    }

    pthread_mutex_unlock(&pRing->m);
    pthread_mutex_destroy(&pRing->m);

    return RING_BUF_PURE_C_SUCCESS;
}

/******************************************************
 * 函数功能: 扩展环形队列的容量, 并保持以前的数据不变
 * ---------------------------------------------------
 * @param - pRing, 环形队列指针
 * @param - size, 扩展后的容量大小, 其值要比以前的容量大
 * ---------------------------------------------------
 * @return - 成功, 返回扩展后的容量大小.
 *           失败, 返回失败代码
 ******************************************************/
int extendCap(ringBuf_p pRing, int size)
{
    if (NULL == pRing)
    {
        return RING_BUF_PURE_C_PARAM_ERROR;
    }

    if (size <= pRing->capacity)
    {
        return RING_BUF_PURE_C_PARAM_ERROR;
    }

    u8 *pbuf = (u8*) calloc(size, sizeof(u8));
    if (pbuf == NULL)
    {
        return RING_BUF_PURE_C_FATAL_ERROR;
    }

    pthread_mutex_trylock(&pRing->m);

    if (pRing->pbuf != NULL && pRing->capacity > 0)
    {
        int i = 0;
        for (i = 0; i < pRing->curSize; i++)
        {
            ringBuf_popData(pRing, &pbuf[i], 1);
        }

        pRing->rPos = 0;
        pRing->wPos = pRing->curSize;

        free(pRing->pbuf);
        pRing->pbuf = NULL;
    }

    pRing->capacity = size;
    pRing->pbuf = pbuf;

    pthread_mutex_unlock(&pRing->m);

    return size;
}
