#ifndef _DLT6485_H
#define _DLT6485_H

#include "basedef.h"

#define DLT645_07_PREFIX            0xFE    //前导符
#define DLT645_07_EVENT_PREFIX      0XFD    //事件上报前导符
#define DLT645_07_PREFIX_COUNT      4       //前导符个数
#define DLT645_07_ADDR_LEN          6       //逻辑地址长度

#define DLT645_07_START_CODE        0x68    //dlt645-07 开始字
#define DLT645_07_END_CODE          0x16    //dlt645-07 结束字
#define DLT645_07_SCAMBLE_CODE      0x33    //dlt645-07 扰码
#define DLT645_07_SCAMBLE_CODE_U16  0x3333  //2字节扰码
#define DLT645_07_IS_COMBCD(x)      (((((x)>>4)&0x0F) <= 9u) && (((x)&0x0F) <= 9u))

#define DLT645_07_WILDCHAR_ADDR  	0xAA    //通配地址
#define DLT645_07_BUF_LEN			256		//645报文中数据域的最大长度

#pragma pack(push)
#pragma pack(1)

typedef union           //645-07协议控制码结构
{
    u8 u8b;
    struct {
#define DLT645_07_CTL_REV         0x00  //保留
#define DLT645_07_CTL_SEC_AUT     0x03  //安全认证security authentication
#define DLT645_07_CTL_BROADCAST   0x08  //广播校时
#define DLT645_07_CTL_RDATA       0x11  //读数据, 也用来透明转发或转发cj188报文
#define DLT645_07_CTL_RSUCD       0x12  //读后续数据
#define DLT645_07_CTL_RADDR       0x13  //读通信地址
#define DLT645_07_CTL_WDATA       0x14  //写数据
#define DLT645_07_CTL_WADDR       0x15  //写通信地址
#define DLT645_07_CTL_FRZ         0x16  //冻结命令
#define DLT645_07_CTL_WBAUD       0x17  //更改通信速率
#define DLT645_07_CTL_WPASSWD     0x18  //修改密码
#define DLT645_07_CTL_CLRDEM      0x19  //最大需量清零
#define DLT645_07_CTL_CLRMETER    0x1A  //电表清零
#define DLT645_07_CTL_CLRENT      0x1B  //事件清零
#define DLT645_07_CTL_VALVE       0x1C  //跳闸, 报警, 保电
#define DLT645_07_CTL_PORTOUT     0x1D  //多功能端子输出
#define DLT645_07_CTL_FILETRANS   0x1E  //文件传输, 中电联未对文件传输的控制码做出规定, 暂时用0x1E
#define DLT645_07_CTL_ADDR_DET    0X1F  //用于与从载波地址探测交互
#define DLT645_07_CTL_ADDR_READ   0X13  //用于与从载波地址探测交互, 用0x13广播读取
        u8 func :5;                //功能码
#define DLT645_07_CTLNOSUCCEED   0      //无后续数据帧
#define DLT645_07_CTLSUCCEED     1      //有后续数据帧
        u8 succeed :1;                //有无后继帧, 0: 无后续数据帧, 1: 有后续数据帧
#define DLT645_07_CTLVALID       0      //从站正确应答
#define DLT645_07_CTLINVALID     1      //从站异常应答
        u8 valid :1;                //从站应答, 0: 从站正确应答, 1: 从站异常应答
#define DLT645_07_CTLDOWN        0      //主站发出的命令帧
#define DLT645_07_CTLUP          1      //从站发出的应答帧
        u8 dir :1;                //传输方向, 0: 主站发出的命令帧, 1: 从站发出的应答帧
    } ctl;
} ctl645_u;

/**
 * dlt645-07 头部格式
 */
typedef struct {
    u8 start1;       //第一个开始字
    u8 addr[DLT645_07_ADDR_LEN];      //逻辑地址, 小端排列
    u8 start2;       //第二个开始字
    ctl645_u ctl;          //控制字
    u8 len;          //数据域的字节数。读数据时 L≤200，写数据时 L≤50， L=0 表示无数据域
} head645_s;

#define DLT645_07_HEAD_LEN  sizeof(head645_s)               //645-07报文, 头部的长度
#define DLT645_07_MIN_FRAME_LEN (DLT645_07_HEAD_LEN + 2)    //645-07报文的最小长度, 不包括前导符. 头部长度 + 1个校验字 + 1个结束符

typedef union      //645-07协议数据标识符结构
{
    u32 u32b;
    u8 list[4];
    struct {
        u8 di0;     //标识符第0字节
        u8 di1;     //标识符第1字节
        u8 di2;     //标识符第2字节
        u8 di3;     //标识符第3字节
    } di;
    struct {
        u8 diA1;    //电能量索引-0x00
        u8 diItem;  //电能量内数据项索引
        u8 diRate;  //电能量费率索引, 最大0x3F
        u8 diDate;  //电能量上n结算日, 最大0x0C
    } diA1;
} di645_u;

typedef struct {
    head645_s head;
    u8 data[DLT645_07_BUF_LEN];
    u8 cs;
    u8 end;
} dlt645_s;
typedef dlt645_s *dlt645_p;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

extern int dlt64507_is645Frame(u8 *pbuf, u16 len, u16 *frmLen, u16 *start);
extern u8 *dlt64507_removePrefix(u8 *pbuf, u16 len, u16 *pLen);
extern u16 dlt64507_compose(dlt645_p pDlt645, u8 *buf, u16 bufSize);
extern void dlt64507_getAddrFromBuf(u8 *pbuf, u16 len, u8 *pAddr, u8 inverse);
extern int dlt64507_getOne645Frame(u8 *buf, u16 bufSize, u16 *pLen);

#ifdef __cplusplus
}
#endif

#endif
