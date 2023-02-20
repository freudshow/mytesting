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

#include "list.h"
#include "basedef.h"
#include "oled.h"
#include "tcp.h"
#include "sqlite3/sqlite3.h"

//int getBlob(sqlite3 *db, int unicode)
//{
//	char sql[128];
//	sqlite3_stmt *stmt;
//
//	snprintf(sql, 127, "select font from t_unicode_gbk where unicode=%d;",
//			unicode);
//
//	//读取数据
//	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
//	int result = sqlite3_step(stmt);
//	int len = 0;
//	while (result == SQLITE_ROW)
//	{
//		const char *pReadBolbData = sqlite3_column_blob(stmt, 0);
//		len = sqlite3_column_bytes(stmt, 0);
//
//		DEBUG_BUFF_FORMAT(pReadBolbData, len, "");
//		result = sqlite3_step(stmt);
//	}
//
//	sqlite3_finalize(stmt);
//
//	return 0;
//}


#define MODE_UDP_SERVER      0X80
#define MODE_UDP_CLIENT      0X81
#define MODE_TCP_SERVER      0X82
#define MODE_TCP_CLIENT      0X83
#define  Net_BUF_SIZE  4096
#define  NetChannelDataScanTime 10  //单位ms
#define  NetRcvIdleCount 10
#define  NET_STRING_LEN 64          //网络相关字符串长度
#define  NET_INTERFACE_COUNT 3      //网络接口数量
#define  NetMaxCount   4
#define  LinkMaxNum  20

enum   NetMode
{
    NetMode_disable = 0, //不使能网络
    NetMode_enableTcpClient = 131, // 使能tcp客户端模式
    NetMode_enableTcpServer = 130, // 使能tcp服务器端模式
    NetMode_enableUdpClient = 129, // 使能udp客户端模式
    NetMode_enableUdpServer = 128 // 使能udp服务器端模式
};


enum SocketChannel
{
    SocketChannel_0       = 0,
    SocketChannel_1,
    SocketChannel_2,
    SocketChannel_3,
    SocketChannel_4,
    SocketChannel_5,
    SocketChannel_6,
    SocketChannel_7,
    SocketChannel_NoSelect = 8
};


#pragma pack(push)
#pragma pack(1)

typedef struct TETHNET_MAC
{
	unsigned char bMac[6];

    //运行参数
    unsigned char bEnable;
    unsigned char bRsv;
}TETHNET_MAC;

typedef struct TETHNET_PORTS
{
	unsigned char bGateWayIP[4];
	unsigned char bSelfIP[4];
	unsigned char bSelfMask[4];
}TETHNET_PORTS;

typedef struct TETHNET_CHANELS
{
	unsigned char  bSideIP[4];//  远程的 ip地址
    unsigned char  bSelfIp[4];//自身的IP地址
	unsigned short int wSidePort; // 远程端口
	unsigned short int wSelfPort; // 自身端口
	unsigned short int wMode;          //D0=1,client；D0=0,server；D1=1,TCP；D1=0,UDP;D7=1,enable，D7=0,disable
}TETHNET_CHANELS;

/* TCP服务器模式时，要记录允许连接的客户端IP */
typedef struct  TcpServerSet
{
    unsigned char No;            // 客户端序号
    unsigned char ClientIP[20];  // 客户端IP
} t_TcpServerSet, *pt_TcpServerSet;

typedef struct  ChildNetCfg
{
    unsigned char linkno;
    unsigned char mode; // 0 不使能网络，1 使能tcp客户端模式，2 使能tcp服务器端模式 3 使能UDP客户端模式，4 使能udp服务器端模式
    unsigned char ipaddr[20]; // IP 地址  4个字节
    unsigned short int LocalPort;
    unsigned short int RemotePort;
    unsigned char SocketChannelNo ;
    unsigned char ifNo;
	pt_TcpServerSet ptTcpServerSet;  //作为TCP服务器时，需要记录允许连接的客户端IP
	unsigned char TcpClientCount;    //作为TCP服务器时，允许连接的客户端IP的个数
} t_ChildNetCfg, *pt_ChildNetCfg;


typedef struct NetInterface {
    unsigned char netIdx;
    unsigned char ifName[NET_STRING_LEN]; //网络接口名, 如"eth0", "FE0", "ppp-0"等
    unsigned char mac[25];
    unsigned char macChanged;// 0不适用界面配置的参数，1使用界面配置的mac 参数
    unsigned char localIPAddr[20];
    unsigned char gateway[20];
    unsigned char subnet[20];
    unsigned char dns_server[20];
    unsigned char netIndex;
}NetInterface_s;

typedef struct NetCfg
{

    //unsigned char  *mac  ; // mac 地址
    //t_ChildNetCfg * pt_GridNetCfg ;
    unsigned char mac[25];
    unsigned char macChanged;// 0不适用界面配置的参数，1使用界面配置的mac 参数
    unsigned char localIPAddr[20];
    unsigned char gateway[20];
    unsigned char subnet[20];
    unsigned char dns_server[20];

    NetInterface_s ifList[NET_INTERFACE_COUNT];

    struct  ChildNetCfg   ChildNodeCfg[LinkMaxNum] ;
} t_NetCfg, *pt_NetCfg ;



/*循环缓冲区*/
typedef struct              /*必须和VBuf保持一致*/
{
    pthread_mutex_t m;
    //signed short	size;			/*缓冲区大小*/
    unsigned short	rp;				/*读偏移*/
    unsigned short	wp;				/*写偏移*/
    //unsigned short  Rev;
    unsigned char	 addr[Net_BUF_SIZE];			/*缓冲区地址*/
} VNetBuf;



/*逻辑通道相关*/
typedef struct
{
	unsigned int  used;			        	/*是否有对应的物理口*/
	VNetBuf       rx_buf;
	VNetBuf       tx_buf;
    int           ifIdx;                    //网口索引号
    int           portIdx;                  //端口索引号
    int           linkNo;                   //链路号
    unsigned char rx_idle;           /**<接收数据后,等rx_idle时间,若期间无数据,则认为接收完成*/

#ifdef DevTypeC0

    printFlag_s   print_flag;

#endif
 ////////////////此通道对应的网路配置信息////////////////////////////////////////
    TETHNET_CHANELS  tEthChanel;
 	TETHNET_PORTS tEthPort;
    TETHNET_MAC tMac;
	pt_TcpServerSet ptTcpServerSet;  //作为TCP服务器时，需要记录允许连接的客户端IP
	unsigned char TcpClientCount;    //作为TCP服务器时，允许连接的客户端IP的个数
	unsigned char TcpConnectOK;      //作为TCP服务器时，同一时间只能连一个客户端，此标志为是否已经有客户端连接。
} VNet;
#pragma pack(pop)

#define ROW	2
#define COL	2

union {
  	char a[10];
  	int i;
  } u;
int main(int argc, char **argv)
{
#if defined(__GNUC__)
# if defined(__i386__)
    /* Enable Alignment Checking on x86 */
    __asm__("pushf\norl $0x40000,(%esp)\npopf");
# elif defined(__x86_64__)
     /* Enable Alignment Checking on x86_64 */
    __asm__("pushf\norl $0x40000,(%rsp)\npopf");
# endif
#endif
//
//	size_t len = ROW*COL*sizeof(VNet);
//	VNet *pmtx = (VNet*)malloc(len);
//	memset(pmtx, 0, len);
//
//	pthread_t Thread_id;
//	int i = 0, j = 0;
//	for(i = 0; i < ROW; i++)
//	{
//		for(j = 0; j < COL; j++)
//		{
//			g_mtx[i][j] = &pmtx[i*COL+j];
//			pthread_create(&Thread_id, NULL, &func, g_mtx[i][j]);
//		}
//	}
//
//	while(1)
//	{
//		sleep(1);
//	}

    int *p= (int*) &(u.a[1]);
    *p = 17; /* the misaligned addr in p causes a bus error! */

	return 0;
}
