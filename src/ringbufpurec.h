#ifndef __COMMON_RING_BUF_PURE_C_PURE_C_H__
#define __COMMON_RING_BUF_PURE_C_PURE_C_H__

#include "basedef.h"  //获取类型定义: u32, u16, u8等
#include <pthread.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define RING_BUF_PURE_C_TRUE   0           //bool真
#define RING_BUF_PURE_C_FALSE  1           //bool假

#define RING_BUF_PURE_C_SUCCESS  1         //执行成功
#define RING_BUF_PURE_C_PARAM_ERROR  -99   //参数错误
#define RING_BUF_PURE_C_FATAL_ERROR  -100  //致命错误

#define RING_BUF_PURE_C_MOVE_NEXT_POS(pos, cap) do {\
                                                pos = ((pos + 1) % cap);\
                                            }while(0)

#define RING_BUF_PURE_C_MIN(a, b)  (((a) <= (b)) ? (a) : (b))

/**
 * 环形缓冲区类型.
 * 使用int型作为缓冲区的读指针,写指针,
 * 容量, 长度类型等, 是考虑到接口函数
 * 要通过负数返回错误代码, 正数返回长
 * 度.
 * 注意到int类型的取值范围为:
 * -2147483648~2147483647,
 * 所以传给接口函数的长度值, 不要超过
 * 2147483647.
 */
typedef struct ringBufStruct
{
    pthread_mutex_t m;              //线程锁, 用于多线程同步
    int wPos;                       //写指针, 始终指向第一个可写的位置
    int rPos;                       //读指针, 始终指向第一个可读的位置
    int capacity;                   //缓冲区总容量
    int curSize;                    //缓冲区当前元素个数
    u32 idle;                       //针对接收缓冲区, 当接收到新数据时, 
                                    //置为特定值, 下一次调度时--, 
                                    //当idle变为0时, 发接收数据事件
    u8 *pbuf;                       //缓冲区指针
} ringBuf_s;
typedef ringBuf_s* ringBuf_p;

extern int ringBuf_init(ringBuf_p pRing, int capacity);                //初始化环形队列
extern int ringBuf_isEmpty(ringBuf_p pRing);                          //判断队列是否已空
extern int ringBuf_isFull(ringBuf_p pRing);                           //判断队列是否已满
extern int ringBuf_getCapacity(ringBuf_p pRing);                 //获取队列最大容量
extern int ringBuf_getIdle(ringBuf_p pRing);                         //获取队列空闲长度
extern int ringBuf_getSize(ringBuf_p pRing);                        //获取队列已被占用长度
extern int ringBuf_readData(ringBuf_p pRing, u8 *buf, int len);  //读取数据, 但不移动读指针
extern int ringBuf_pushData(ringBuf_p pRing, u8 *buf, int len);    //写入数据
extern int ringBuf_popData(ringBuf_p pRing, u8 *buf, int len);    //读取数据, 且移动读指针
extern int ringBuf_clear(ringBuf_p pRing);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif //_RING_BUF_PURE_C_H__
