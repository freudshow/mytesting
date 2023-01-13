#ifndef BASEDEF_H
#define BASEDEF_H

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

typedef unsigned char boolean; /* bool value, 0-false, 1-true       */
typedef unsigned char u8; /* Unsigned  8 bit quantity          */
typedef char s8; /* Signed    8 bit quantity          */
typedef unsigned short u16; /* Unsigned 16 bit quantity          */
typedef signed short s16; /* Signed   16 bit quantity          */
typedef unsigned int u32; /* Unsigned 32 bit quantity          */
typedef signed int s32; /* Signed   32 bit quantity          */
typedef unsigned long long u64; /* Unsigned 64 bit quantity   	   */
typedef signed long long s64; /* Unsigned 64 bit quantity          */
typedef float fp32; /* Single precision floating point   */
typedef double fp64; /* Double precision floating point   */

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

#define UINT8	u8
#define UINT32	u32
#define INT32 	s32

#define OFFSETOF(type, member) ((size_t)&(((type *)0)->member))

//typeof用于获取表达式的数据类型
//定义一个变量__mptr,它的类型和member相同，const typeof(((type *)0)->member) * __mptr = (ptr)是为了获取实际分配的member的地址
//指针ptr为type类型结构体的member变量
//实际的member的地址减去其在结构体中的偏移得到该结构体的实际分配的地址
#define CONTAINER_OF(ptr, type, member) ({                \
    const typeof(((type *)0)->member) * __mptr = (ptr);   \
    (type *)((char *)__mptr - OFFSETOF(type, member)); })

#define FILE_LINE       __FILE__,__FUNCTION__,__LINE__
//格式化打印日志, 带报文输出
#define DEBUG_BUFFFMT_TO_FILE(fname, buf, bufSize, format, ...) do {\
                                                                        FILE *fp = fopen(fname, "a+");\
                                                                        if (fp != NULL) {\
                                                                            debugBufFormat2fp(fp, FILE_LINE, (char*)buf, bufSize, format, ##__VA_ARGS__);\
                                                                        }\
                                                                        logLimit(fname, LOG_SIZE, LOG_COUNT);\
                                                                    } while(0)

//格式化打印日志
#define DEBUG_TO_FILE(fname, format, ...)   do {\
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

#define DEBUG_BUFF_FORMAT(buf, bufSize, format, ...)    debugBufFormat2fp(stdout, FILE_LINE, (char*)buf, (int)bufSize, format, ##__VA_ARGS__)
#define DEBUG_TIME_LINE(format, ...)    DEBUG_BUFF_FORMAT(NULL, 0, format, ##__VA_ARGS__)

#define POLY                0xA001  //CRC16校验中的生成多项式
#define MODBUS_PROTOCOL     0

/**
 * 声明一个列表
 */
#define A_LIST_OF(type)                     \
    struct {                                \
        unsigned int  capacity; /*当前列表的最大容量*/       \
        unsigned int  count;    /*当前列表的元素个数*/       \
        unsigned int  idx;     /*当前元素索引*/       \
        type *list;    /*列表*/               \
        void (*free)(void *);/*释放列表*/     \
    }

/**
 * 初始化一个列表
 */
#define INIT_LIST(vlist, type, quantity, freeFunc)   \
    do {\
        vlist.capacity = 0;\
        vlist.count = 0;\
        vlist.idx = 0;\
        vlist.list = (type*)calloc(quantity, sizeof(type));\
        if(vlist.list != NULL) {\
            vlist.capacity = quantity;\
        }\
        vlist.free = freeFunc;\
    } while(0)

/**
 * 列表扩容
 */
#define EXTEND_LIST(vlist, type, delta)   \
    do {\
        type *p = (type*)malloc((vlist.capacity + delta)*sizeof(type));\
        if(p != NULL) {\
            memcpy(p, vlist.list, vlist.count*sizeof(type));\
            vlist.free(vlist.list);\
            vlist.list = p;\
            vlist.capacity += delta;\
        }\
    }while(0)

/**
 * 将元素插入列表的索引idx后
 * vlist - 列表
 * idx - 在此索引后插入元素
 * item - 欲插入的元素
 * delta - 如果插入时需要扩容, 则扩容的大小
 * type - 元素类型
 * tempit - 临时的循环变量, 是为了避免变量名冲突
 */
#define INSERT_ITEM_TO_LIST(vlist, idx, item, delta, type, tempit)   \
        do {\
                if (idx < 0 || idx >= vlist.count)\
                {\
                    break;\
                }\
                if ((vlist.count + 1) > vlist.capacity)\
                {\
                    EXTEND_LIST(vlist, type, delta);\
                }\
                for (tempit = vlist.count; tempit > ((idx) + 1); tempit--)\
                {\
                    vlist.list[tempit] = vlist.list[tempit - 1];\
                    printf("--tempit: %d--\n", tempit);\
                }\
                vlist.list[tempit] = item;\
                vlist.count++;\
            } while (0)

/**
 * 刪除一個元素
 * vlist - 列表
 * Index - 要删除的索引号， 注意 从0开始
 * tempit - 临时的循环变量, 是为了避免变量名冲突
 */
#define DELETE_LIST_ONE(vlist, Index, tempit)  \
        do{\
            if (Index < 0 || Index >= vlist.count)\
            {\
                break;\
            }\
            for (tempit = Index; tempit < vlist.count - 1; tempit++)\
            {\
                vlist.list[tempit] = vlist.list[tempit + 1];\
            }\
            vlist.count--;\
        }while(0)

void get_local_time(char *buf, u32 bufLen);
void getBufString(char *buf, int len, char *out);
void debugBufFormat2fp(FILE *fp, const char *file, const char *func, int line,
        char *buf, int len, const char *fmt, ...);
void printfloat(unsigned char *pFloat);
void inverseArray(u8 *buf, int bufSize);
int myprintk(const char *fmt, ...);
void add33(u8 *buf, int bufSize);
void minus33(u8 *buf, int bufSize);
u8 chkSum(u8 *buf, u16 bufSize);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif // BASEDEF_H
