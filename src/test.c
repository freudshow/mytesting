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
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <error.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>
#include "list.h"
#include "basedef.h"



#define OFFSETOF(type, member) ((size_t)&(((type *)0)->member))

//typeof用于获取表达式的数据类型
//定义一个变量__mptr,它的类型和member相同，const typeof(((type *)0)->member) * __mptr = (ptr)是为了获取实际分配的member的地址
//指针ptr为type类型结构体的member变量
//实际的member的地址减去其在结构体中的偏移得到该结构体的实际分配的地址
#define CONTAINER_OF(ptr, type, member) ({                \
	const typeof(((type *)0)->member) * __mptr = (ptr);	  \
	(type *)((char *)__mptr - OFFSETOF(type, member)); })

#pragma pack(push)
#pragma pack(1)

#define DLT645_LOAD_DATA_START 0xA0 //负荷记录起始码
#define DLT645_LOAD_DATA_SPLIT 0xAA //块分隔码
#define DLT645_LOAD_DATA_END 0xE5   //负荷记录结束码

typedef struct dlt645LoadDataClass1
{
	u8 Voltage_A[2]; // A相电压
	u8 Voltage_B[2]; // B相电压
	u8 Voltage_C[2]; // C相电压
	u8 Current_A[3]; // A相电流
	u8 Current_B[3]; // B相电流
	u8 Current_C[3]; // C相电流
	u8 Hz[2];        //电网频率
} loadDataClass1_s;

typedef struct dlt645LoadDataClass2
{
	u8 activePower_tot[3];   // 总有功功率
	u8 activePower_A[3];     // A相有功功率
	u8 activePower_B[3];     // B相有功功率
	u8 activePower_C[3];     // C相有功功率
	u8 reactivePower_tot[3]; // 总无功功率
	u8 reactivePower_A[3];   // A相无功功率
	u8 reactivePower_B[3];   // B相无功功率
	u8 reactivePower_C[3];   // C相无功功率
} loadDataClass2_s;

typedef struct dlt645LoadDataClass3
{
	u8 powerFactor_tot[2]; // 总功率因数
	u8 powerFactor_A[2];   // A相功率因数
	u8 powerFactor_B[2];   // B相功率因数
	u8 powerFactor_C[2];   // C相功率因数
} loadDataClass3_s;

typedef struct dlt645LoadDataClass4
{
	u8 positiveActiveTotalEnergy[4]; // 正向有功总电能
	u8 negetiveActiveTotalEnergy[4]; // 反向有功总电能
	u8 CombineReactiveEnergy1[4];    // 组合无功1总电能, 一般为正向无功总电能
	u8 CombineReactiveEnergy2[4];    // 组合无功2总电能, 一般为反向无功总电能
} loadDataClass4_s;

typedef struct dlt645LoadDataClass5
{
	u8 negetiveReactiveTotalEnergy_quadrant1[4]; // 第一象限无功总电能
	u8 negetiveReactiveTotalEnergy_quadrant2[4]; // 第二象限无功总电能
	u8 negetiveReactiveTotalEnergy_quadrant3[4]; // 第三象限无功总电能
	u8 negetiveReactiveTotalEnergy_quadrant4[4]; // 第四象限无功总电能
} loadDataClass5_s;

typedef struct dlt645LoadDataClass6
{
	u8 activeDemand[3];   // 当前有功需量
	u8 reactiveDemand[3]; // 当前无功需量
} loadDataClass6_s;

typedef struct dlt645LoadDataBlock
{
	u8 startcode[2]; // 负荷记录起始码
	u8 loadLen;      // 负荷记录字节数
	u8 minute;       // 负荷记录存储时间, 分
	u8 hour;         // 负荷记录存储时间, 小时
	u8 day;          // 负荷记录存储时间, 日
	u8 month;        // 负荷记录存储时间, 月
	u8 year;         // 负荷记录存储时间, 年
	loadDataClass1_s class1;
	u8 split1;
	loadDataClass2_s class2;
	u8 split2;
	loadDataClass3_s class3;
	u8 split3;
	loadDataClass4_s class4;
	u8 split4;
	loadDataClass5_s class5;
	u8 split5;
	loadDataClass6_s class6;
	u8 split6;
	u8 chk;
	u8 end;
} dlt645LoadDataBlock_s;

#pragma pack(pop)

#define	FILE_LINE		__FILE__,__FUNCTION__,__LINE__
//格式化打印日志, 带报文输出
#define DEBUG_BUFFFMT_TO_FILE(fname, buf, bufSize, format, ...) do {\
																		FILE *fp = fopen(fname, "a+");\
																		if (fp != NULL) {\
																			debugBufFormat2fp(fp, FILE_LINE, (char*)buf, bufSize, format, ##__VA_ARGS__);\
																		}\
																		logLimit(fname, LOG_SIZE, LOG_COUNT);\
																	} while(0)

//格式化打印日志
#define DEBUG_TO_FILE(fname, format, ...)	do {\
												FILE *fp = fopen(fname, "a+");\
												if (fp != NULL) {\
													debugBufFormat2fp(fp, FILE_LINE, NULL, 0, format, ##__VA_ARGS__);\
												}\
												logLimit(fname, LOG_SIZE, LOG_COUNT);\
											} while(0)

#define DEBUG_BUFFFMT_TO_LOG_FILE(buf, bufSize, format, ...)    do {\
                                                                    char logfilename[256] = { 0 };\
                                                                    getLogFileName(logfilename, sizeof(logfilename));\
                                                                    FILE *fp = fopen(logfilename, "a+");\
                                                                    if (fp != NULL) {\
                                                                        debugBufFormat2fp(fp, FILE_LINE, (char*)buf, bufSize, format, ##__VA_ARGS__);\
                                                                    }\
                                                                    logLimit(logfilename, LOG_SIZE, LOG_COUNT);\
                                                                } while(0)

#define DEBUG_TO_LOG_FILE(format, ...)  DEBUG_BUFFFMT_TO_LOG_FILE(NULL, 0, format, ##__VA_ARGS__)

#define DEBUG_BUFFFMT_TO_SYS_LOG_FILE(buf, bufSize, format, ...)    do {\
                                                                        char logfilename[256] = { 0 };\
                                                                        getSysLogFileName(logfilename, sizeof(logfilename));\
                                                                        FILE *fp = fopen(logfilename, "a+");\
                                                                        if (fp != NULL) {\
                                                                            debugBufFormat2fp(fp, FILE_LINE, (char*)buf, bufSize, format, ##__VA_ARGS__);\
                                                                        }\
                                                                        logLimit(logfilename, LOG_SIZE, LOG_COUNT);\
                                                                    } while(0)

#define DEBUG_TO_SYS_LOG_FILE(format, ...)  DEBUG_BUFFFMT_TO_SYS_LOG_FILE(NULL, 0, format, ##__VA_ARGS__)

#define	DEBUG_BUFF_FORMAT(buf, bufSize, format, ...)	debugBufFormat2fp(stdout, FILE_LINE, (char*)buf, (int)bufSize, format, ##__VA_ARGS__)
#define	DEBUG_TIME_LINE(format, ...)	DEBUG_BUFF_FORMAT(NULL, 0, format, ##__VA_ARGS__)

/**************************************************
 * 功能描述: 获得当前时间字符串
 * ------------------------------------------------
 * 输入参数: buffer [out]: 当前时间的字符串, 长度
 *  	    至少为20个字节, 用以存储格式为"2020-07-01 16:13:33"
 * 输出参数: buffer
 * ------------------------------------------------
 * 返回值: 无
 * ------------------------------------------------
 * 修改日志:
 * 		1. 日期: 2020年7月19日
 创建函数
 **************************************************/
void get_local_time(char *buf, u32 bufLen)
{
	struct timeval systime;
	struct tm timeinfo;

	gettimeofday(&systime, NULL);
	localtime_r(&systime.tv_sec, &timeinfo);
	snprintf(buf, bufLen, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
			(timeinfo.tm_year + 1900), timeinfo.tm_mon + 1, timeinfo.tm_mday,
			timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
			systime.tv_usec / 1000);
}

/**************************************************
 * 功能描述: 将字节数组, 以16进制格式, 打印到字符串out中, 以空格
 * 分开每个字节
 * ------------------------------------------------
 * 输入参数: buf [in]: 字节数组
 * 输入参数: len [in]: 字节数组长度
 * 输出参数: out, 输出的字符串
 * ------------------------------------------------
 * 返回值: 无
 * ------------------------------------------------
 * 修改日志:
 * 		1. 日期: 2020年7月19日
 创建函数
 **************************************************/
void getBufString(char *buf, int len, char *out)
{
	int i = 0;
	char s[5] = { 0 };
	for (i = 0; i < len; i++)
	{
		sprintf(s, "%02X ", (unsigned char) buf[i]);
		strcat(out, s);
	}
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
 * 		1. 日期: 2020年7月19日
 创建函数
 **************************************************/
void debugBufFormat2fp(FILE *fp, const char *file, const char *func, int line,
		char *buf, int len, const char *fmt, ...)
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
			char *s = (char*) calloc(1, 3 * (len + 1)); //多开3个字符的余量
			if (s != NULL)
			{
				getBufString(buf, len, s);
				fprintf(fp, "%s", s);
				free(s);
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

#define POLY		        0xA001	//CRC16校验中的生成多项式
#define MODBUS_PROTOCOL     0

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
 * 		1. 日期: 2020年7月19日
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
			} else
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
	} else
	{
		pstr = p;
	}

	sLen = strlen(pstr);

	for (i = 0; i < sLen; i++)
	{
		if ((pstr[i] < '0') || ((pstr[i] > '9') && (pstr[i] < 'A'))
				|| ((pstr[i] > 'F') && (pstr[i] < 'a')) || (pstr[i] > 'f'))
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
		} else if ((pstr[i] >= 'A') && (pstr[i] <= 'F'))
		{
			tmpValue = pstr[i] - 'A' + 0x0A;
		} else if ((pstr[i] >= 'a') && (pstr[i] <= 'f'))
		{
			tmpValue = pstr[i] - 'a' + 0x0A;
		} else
		{
			tmpValue = 0;
		}

		if ((j % 2) == 0)
		{
			pbcd[m] = tmpValue;
		} else
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
	} else if (order == 0)
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
	} else
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

s8 bcd2floatBlock(u8 *bcd, u8 lenPerItem, u8 itemCount, u8 order, u8 signOnHigh,
		u8 declen, float *dfloat)
{
	if (dfloat == NULL)
	{
		return -1;
	}

	int i = 0;
	s8 res = 0;

	for (i = 0; i < itemCount; i++)
	{
		res = bcd2float(bcd + lenPerItem * i, lenPerItem, order, signOnHigh,
				declen, dfloat + i);
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
		} else
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

int SetNonBlocking(int sockfd)
{
	int flag = fcntl(sockfd, F_GETFL, 0);
	if (flag < 0)
	{
		return -1;
	}

	if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0)
	{
		return -1;
	}

	return 0;
}

int tcpclient()
{
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sd)
	{
		printf("socket error!\n");
		myprintk("Socke_0_callback_pthread create socket err");
		return -1;
	}

	int res = SetNonBlocking(sd);
	if (res < 0)
	{
		printf("SetNonBlocking error!\n");
		return -1;
	}

	int len = 0;
	int netport = 7788;
	char setip[30];
	memset(setip, '\0', 30);
	strcpy(setip, "192.168.31.217");

	struct sockaddr_in tSocketServerAddr;
	tSocketServerAddr.sin_family = AF_INET;
	tSocketServerAddr.sin_port = htons(netport);
	tSocketServerAddr.sin_addr.s_addr = inet_addr(setip);

	memset(tSocketServerAddr.sin_zero, 0, 8);

//	errno = 0;
	int iRet = connect(sd, (const struct sockaddr*) &tSocketServerAddr,
			sizeof(struct sockaddr));
	if (iRet == -1)
	{
		if (errno != EINPROGRESS)
		{
			printf("TCP_CLIENT connect error!channel= %d\n", netport);
			printf("connect: %s\n", strerror(errno));
			close(sd);
			return -1;
		}
	}

	if (iRet == 0)
	{
		goto done;
	}

	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	fd_set wset;
	FD_SET(sd, &wset);

	if ((res = select(sd + 1, NULL, &wset, NULL, &timeout)) <= 0)  // TIMEOUT
	{
		close(sd);
		errno = ETIMEDOUT;
		return (-1);
	}

	if (!FD_ISSET(sd, &wset))
	{
		printf("no events on sockfd found\n");
		close(sd);
		return -1;
	}

	int error = 0;
	socklen_t length = sizeof(error);
	// 调用 getsockopt 来获取并清除 sockfd 上的错误
	if (getsockopt(sd, SOL_SOCKET, SO_ERROR, &error, &length) < 0)
	{
		printf("get socket option failedn");
		close(sd);
		return -1;
	}

	// 错误号不为 0 表示连接出错
	if (error != 0)
	{
		printf("connection failed after select with the error: %sn",
				strerror(error));
		close(sd);
		return -1;
	}

	done:
	/////////////// 网络断线重连机制 设定/////////////////////////
	int keepIdle = 10;
	int keepInterval = 5;
	int keepcount = 2;
	int keepAlive = 1;

	setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (void*) &keepAlive,
			sizeof(keepAlive));
	setsockopt(sd, 6, TCP_KEEPIDLE, (void*) &keepIdle, sizeof(keepIdle));
	setsockopt(sd, 6, TCP_KEEPINTVL, (void*) &keepInterval,
			sizeof(keepInterval));
	setsockopt(sd, 6, TCP_KEEPCNT, (void*) &keepcount, sizeof(keepcount));

	u8 buf[4096];
	struct tcp_info info;

	while (1)
	{
		len = sizeof(info);
		getsockopt(sd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t*) &len);
		if (info.tcpi_state != 1) //tcp 建立链接
		{
			printf("tcp client disconnect \r\n");
			close(sd);
			return -1;
		}

		len = read(sd, buf, sizeof(buf));
		if (len > 0)
		{
			DEBUG_BUFF_FORMAT(buf, len, "recev: ");
		}

		usleep(1000 * 10);
	}

	return -1;
}

int testtcpclientmain(void)
{
	while (1)
	{
		tcpclient();
		sleep(1);
	}

	return EXIT_SUCCESS;
}

int testip(void)
{
    struct ifaddrs *ifaddr;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    /* Walk through linked list, maintaining head pointer so we
     can free list later. */
    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        if (family != AF_INET && family != AF_INET6)
        {
        	continue;
        }

        /* Display interface name and family (including symbolic
         form of the latter for the common families). */

        DEBUG_TIME_LINE("%-8s %s (%d)\n", ifa->ifa_name, (family == AF_PACKET) ? "AF_PACKET" : (family == AF_INET) ? "AF_INET" : (family == AF_INET6) ? "AF_INET6" : "???", family);

        /* For an AF_INET* interface address, display the address. */
        if (family == AF_INET || family == AF_INET6)
        {
            s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host, NI_MAXHOST,
            NULL, 0, NI_NUMERICHOST);
            if (s != 0)
            {
                DEBUG_TIME_LINE("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }

            DEBUG_TIME_LINE("\t\taddress: <%s>\n", host);
        }
        else if (family == AF_PACKET && ifa->ifa_data != NULL)
        {
            struct rtnl_link_stats *stats = ifa->ifa_data;

            DEBUG_TIME_LINE("\t\ttx_packets = %10u; rx_packets = %10u\n"
                    "\t\ttx_bytes   = %10u; rx_bytes   = %10u\n", stats->tx_packets, stats->rx_packets, stats->tx_bytes, stats->rx_bytes);
        }
    }

    freeifaddrs(ifaddr);

    return 0;
}

int ipmain()
{
    struct ifaddrs *addresses;
    if (getifaddrs(&addresses) == -1)
    {
        printf("getifaddrs call failed\n");
        return -1;
    }

    struct ifaddrs *address = addresses;
    while (address)
    {
        int family = address->ifa_addr->sa_family;
        if (family == AF_INET || family == AF_INET6)
        {
            printf("%s\t", address->ifa_name);
            printf("%s\t", family == AF_INET ? "IPv4" : "IPv6");
            char ap[100];
            const int family_size = family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
            getnameinfo(address->ifa_addr, family_size, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
            printf("\t%s\n", ap);
        }

        address = address->ifa_next;
    }

    freeifaddrs(addresses);

    return 0;
}

int getLocalInfo(void)
{
    int fd;
    int interfaceNum = 0;
    struct ifreq buf[16];
    struct ifconf ifc;
    struct ifreq ifrcopy;
    char mac[16] = { 0 };
    char ip[32] = { 0 };
    char broadAddr[32] = { 0 };
    char subnetMask[32] = { 0 };

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");

        close(fd);
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t) buf;
    if (!ioctl(fd, SIOCGIFCONF, (char*) &ifc)) {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        printf("interface num = %d\n", interfaceNum);
        while (interfaceNum-- > 0) {
            printf("\ndevice name: %s\n", buf[interfaceNum].ifr_name);

            //ignore the interface that not up or not runing
            ifrcopy = buf[interfaceNum];
            if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy)) {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__,
                        __LINE__);

                close(fd);
                return -1;
            }

            //get the mac of this interface
            if (!ioctl(fd, SIOCGIFHWADDR, (char*) (&buf[interfaceNum]))) {
                memset(mac, 0, sizeof(mac));
                snprintf(mac, sizeof(mac), "%02x%02x%02x%02x%02x%02x",
                        (unsigned char) buf[interfaceNum].ifr_hwaddr.sa_data[0],
                        (unsigned char) buf[interfaceNum].ifr_hwaddr.sa_data[1],
                        (unsigned char) buf[interfaceNum].ifr_hwaddr.sa_data[2],

                        (unsigned char) buf[interfaceNum].ifr_hwaddr.sa_data[3],
                        (unsigned char) buf[interfaceNum].ifr_hwaddr.sa_data[4],
                        (unsigned char) buf[interfaceNum].ifr_hwaddr.sa_data[5]);
                printf("device mac: %s\n", mac);
            } else {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__,
                        __LINE__);
                close(fd);
                return -1;
            }

            //get the IP of this interface

            if (!ioctl(fd, SIOCGIFADDR, (char*) &buf[interfaceNum])) {
                snprintf(ip, sizeof(ip), "%s",
                        (char*) inet_ntoa(
                                ((struct sockaddr_in*) &(buf[interfaceNum].ifr_addr))->sin_addr));
                printf("device ip: %s\n", ip);
            } else {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__,
                        __LINE__);
                close(fd);
                return -1;
            }

            //get the broad address of this interface

            if (!ioctl(fd, SIOCGIFBRDADDR, &buf[interfaceNum])) {
                snprintf(broadAddr, sizeof(broadAddr), "%s",
                        (char*) inet_ntoa(
                                ((struct sockaddr_in*) &(buf[interfaceNum].ifr_broadaddr))->sin_addr));
                printf("device broadAddr: %s\n", broadAddr);
            } else {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__,
                        __LINE__);
                close(fd);
                return -1;
            }

            //get the subnet mask of this interface
            if (!ioctl(fd, SIOCGIFNETMASK, &buf[interfaceNum])) {
                snprintf(subnetMask, sizeof(subnetMask), "%s",
                        (char*) inet_ntoa(
                                ((struct sockaddr_in*) &(buf[interfaceNum].ifr_netmask))->sin_addr));
                printf("device subnetMask: %s\n", subnetMask);
            } else {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__,
                        __LINE__);
                close(fd);
                return -1;

            }
        }
    } else {
        printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

int main(int argc, char **argv)
{
//	loadDataClass1_s class1;
//	u8 (*pVoltage_B)[3] = &class1.Current_B;
//	loadDataClass1_s *pclass1 = (loadDataClass1_s*) CONTAINER_OF(pVoltage_B,
//			loadDataClass1_s, Current_B);
//	printf("class1 address: %p, container of b: %p", &class1, pclass1);

//	int count = getMenuSize();
//	pMenuNode pMenuList = ComposeDList(g_menuArray, count);
//	printMenuTree(pMenuList);
//
//	makeEmpty(pMenuList);
//	pMenuList = NULL;
//
//	printf("after make empty\n\n\n\n");
//	printMenuTree(pMenuList);
	getLocalInfo();
//	ipmain();
	return 0;
}
