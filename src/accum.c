#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

#include "basedef.h"
#include "md5.h"

/**
 * 声明一个列表
 */
#define	ACCUM_A_LIST_OF(type)					    \
    struct {					            \
        u32  capacity; /*当前列表的最大容量*/		\
		u32  count;    /*当前列表的元素个数*/		\
        u32  idx;	   /*当前元素索引*/      	\
        type *list;	   /*列表*/			    \
		void (*free)(void *);/*释放列表*/     \
    }

#define ACCUM_STRING_LEN 256    //字符串长度

/**
 * 能量累计类型
 */
typedef enum accumEnergyType {
    e_accumulate_day = 0,           //日冻结
    e_accumulate_month = 1,         //月冻结
    e_accumulate_year = 2,          //年冻结
    e_accumulate_week = 3,          //周冻结
    e_accumulate_type_count = 4,    //冻结类型的个数
    e_accumulate_invalid = 255      //非法类型
}accumEnergyType_e;

typedef enum accumPhaseType {
    e_accum_phase_A = 0,      //A相
    e_accum_phase_B = 1,      //B相
    e_accum_phase_C = 2,      //C相
}accumPhaseType_e;

typedef enum accumResultType {
    e_accumulate_result_total = 0,      //三相总电能
    e_accumulate_result_divide = 1,     //分相电能
    e_accumulate_result_invalid = 255   //非法类型
}accumResultType_e;

typedef struct accumItemStruct{
    int realDbNo;                       //实时数据库号
    float currentValue;                 //当前值
    char description[ACCUM_STRING_LEN]; //描述
}accumItemStruct_s;

typedef struct freezeItemStruct{
        accumEnergyType_e type; //能量累计类型
        int depth;              //存储深度
}freezeItem_s;

typedef struct accumEnergyConfigStruct
{
    int acqPeriod;                                  //采样周期, 单位秒
    accumResultType_e resultType;                   //累计结果类型
    int startRealDbNo;                              //起始实时数据库号
    ACCUM_A_LIST_OF(accumItemStruct_s) accumItemList;     //累计数据项的列表
    ACCUM_A_LIST_OF(freezeItem_s) freezeItemList;         //冻结项的列表
}accumEnergyConfig_s;

#define ACCUM_STRING_LEN 256    //字符串长度

typedef struct  SysTime_t
{
    unsigned short int   Year;
    unsigned char    Month;
    unsigned char    Day;
    unsigned char    Hour;
    unsigned char    Minute;
    unsigned char    Second;
    unsigned int     MSecond;
    unsigned char    weekday;
}SysTime_s;

/**
 * 释放指针
 */
#define ACCUM_FREE_DATA(p)  do{\
                                if (p != NULL) \
                                { \
                                    free(p); \
                                    p = NULL; \
                                }\
                              }while(0)

#define ACCUM_PHASE_COUNT   3               //相电能的个数
#define ACCUM_DATA_FILE     "accum_energy"  //电能冻结数据文件名

/**
 * 1个电能累计数据项
 */
typedef struct accumDataStruct {
    u16 realDbNo;                       //数据项写入的实时数据库号
    int linkNo;                         //link号
    int devNo;                          //设备号
    int regNo;                          //寄存器号
    float freezeValue;                  //数据项的冻结值
    SysTime_s freezeTime;               //数据项的冻结时间
    char description[ACCUM_STRING_LEN]; //数据项的描述
} accumDataItem_s;

/**
 * 电能累计数据列表
 */
typedef ACCUM_A_LIST_OF(accumDataItem_s) accumDataItemList_s;

/**
 * 1各冻结数据项列表
 */
typedef struct accumDataList {
    accumEnergyType_e accumType;        //能量累计类型, 日冻结, 月冻结, 年冻结, 或者周冻结
    accumDataItemList_s dataList;       //累计数据项的列表, 包括当前数据+上N次数据
} accumDataList_s;

/**
 * 1个分相/合相电能列表
 */
typedef struct accumFreezeList {
    u16 srcDbNo;                                    //源实时库号, 比如A相正向有功电能的实时数据库号
    char description[ACCUM_STRING_LEN];             //源数据描述, 比如A相正向有功电能的描述
    ACCUM_A_LIST_OF(accumDataList_s) freezeDataList;      //当前相的数据列表, 包括日冻结, 月冻结, 年冻结和周冻结的当前数据+上N次数据
} accumFreezeList_s;

/**
 * 总的电能冻结列表
 */
typedef struct accumEnergyList {
    u8 jsonMd5[16];                                 //json配置文件的md5校验值
    accumEnergyConfig_s *pEnergyConfig;             //能量累计配置
    ACCUM_A_LIST_OF(accumFreezeList_s) phaseDataList;     //冻结项的列表
    SysTime_s lastTime;                             //上次采样时间
} accumEnergyList_s;


/*********************************************
 * 函数功能: 读取json配置文件
 * ------------------------------------------
 * @param - 无
 * ------------------------------------------
 * @return - 无
 ********************************************/
accumEnergyConfig_s* parse_accumulate_config(const char *configFileName)
{
    json_t *root = NULL;
    json_t *arrayList = NULL;
    json_t *arrayItem = NULL;
    json_t *object_tmp = NULL;
    json_error_t error;

    root = json_load_file((const char*) configFileName, 0, &error);
    if (NULL == root)
    {
        return NULL;
    }

    accumEnergyConfig_s *p_accumEnergyConfig = malloc(sizeof(accumEnergyConfig_s));
    if (NULL == p_accumEnergyConfig)
    {
        DEBUG_TIME_LINE("pad malloc err");
        return NULL;
    }

    memset(p_accumEnergyConfig, 0, sizeof(accumEnergyConfig_s));
    p_accumEnergyConfig->acqPeriod = 2000; //默认10秒钟刷新一次数据
    p_accumEnergyConfig->resultType = e_accumulate_result_total;

    //读取头部信息 start
    object_tmp = json_object_get(root, "acqPeriod");
    p_accumEnergyConfig->acqPeriod = json_integer_value(object_tmp);

    object_tmp = json_object_get(root, "resultStoreType");
    p_accumEnergyConfig->resultType = (accumResultType_e) json_integer_value(object_tmp);

    object_tmp = json_object_get(root, "startRealDbNo");
    p_accumEnergyConfig->startRealDbNo = json_integer_value(object_tmp);
    //读取头部信息 end

    //读取分相电能的数据源实时库号 start
    arrayList = json_object_get(root, "dataSourceList");
    size_t count = json_array_size(arrayList);
    if (count == 0)
    {
        json_decref(root);
        free(p_accumEnergyConfig);
        return NULL;
    }

    p_accumEnergyConfig->accumItemList.list = (accumItemStruct_s*) malloc(count * sizeof(accumItemStruct_s));
    if(p_accumEnergyConfig->accumItemList.list == NULL)
    {
        json_decref(root);
        free(p_accumEnergyConfig);
        return NULL;
    }

    memset(p_accumEnergyConfig->accumItemList.list, 0, count * sizeof(accumItemStruct_s));
    p_accumEnergyConfig->accumItemList.capacity = count;
    p_accumEnergyConfig->accumItemList.count = count;

    int i = 0;
    for (i = 0; i < count; i++)
    {
        arrayItem = json_array_get(arrayList, i);

        object_tmp = json_object_get(arrayItem, "realDbNo");
        p_accumEnergyConfig->accumItemList.list[i].realDbNo = json_integer_value(object_tmp);

        object_tmp = json_object_get(arrayItem, "description");
        strncpy(p_accumEnergyConfig->accumItemList.list[i].description,
                json_string_value(object_tmp),
                sizeof(p_accumEnergyConfig->accumItemList.list[i].description) - 1);
    }

    //读取分相电能的数据源实时库号 end

    //读取冻结类型项目 start
    arrayList = json_object_get(root, "freezeDataList");
    count = json_array_size(arrayList);
    if (count == 0)
    {
        json_decref(root);
        free(p_accumEnergyConfig->accumItemList.list);
        free(p_accumEnergyConfig);
        return NULL;
    }

    p_accumEnergyConfig->freezeItemList.list = (freezeItem_s*) malloc(count * sizeof(freezeItem_s));
    if(p_accumEnergyConfig->freezeItemList.list == NULL)
    {
        json_decref(root);
        free(p_accumEnergyConfig->accumItemList.list);
        free(p_accumEnergyConfig);
        return NULL;
    }

    memset(p_accumEnergyConfig->freezeItemList.list, 0, count * sizeof(freezeItem_s));
    p_accumEnergyConfig->freezeItemList.capacity = count;
    p_accumEnergyConfig->freezeItemList.count = count;

    i = 0;
    for (i = 0; i < count; i++)
    {
        arrayItem = json_array_get(arrayList, i);

        object_tmp = json_object_get(arrayItem, "type");
        p_accumEnergyConfig->freezeItemList.list[i].type = (accumEnergyType_e)json_integer_value(object_tmp);

        object_tmp = json_object_get(arrayItem, "depth");
        p_accumEnergyConfig->freezeItemList.list[i].depth = json_integer_value(object_tmp);
    }

    //读取冻结类型项目 end

    json_decref(root);

    return p_accumEnergyConfig;
}

/*******************************************************
* 函数名称: accum_freeDataItemList
* ---------------------------------------------------
* 功能描述: 释放电能数据项列表指针
* ---------------------------------------------------
* 输入参数: p_accumDataList - 电能数据项列表指针
* ---------------------------------------------------
* 输出参数: 无
* ---------------------------------------------------
* 返回值: 无
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static void accum_freeDataItemList(accumDataList_s *p_accumDataList)
{
    if (p_accumDataList == NULL)
    {
        return;
    }

    ACCUM_FREE_DATA(p_accumDataList->dataList.list);
    p_accumDataList->dataList.capacity = 0;
    p_accumDataList->dataList.count = 0;
    p_accumDataList->dataList.idx = 0;
    p_accumDataList->dataList.free = NULL;
}

/*******************************************************
* 函数名称: accum_freeFreezeList
* ---------------------------------------------------
* 功能描述: 释放单相数据项列表指针
* ---------------------------------------------------
* 输入参数: p_accumFreezeList - 单相数据项列表指针
* ---------------------------------------------------
* 输出参数: 无
* ---------------------------------------------------
* 返回值: 无
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static void accum_freeFreezeList(accumFreezeList_s *p_accumFreezeList)
{
    if (p_accumFreezeList == NULL)
    {
        return;
    }

    int i = 0;
    for (i = 0; i < p_accumFreezeList->freezeDataList.capacity; i++)
    {
        accum_freeDataItemList(&p_accumFreezeList->freezeDataList.list[i]);
    }

    ACCUM_FREE_DATA(p_accumFreezeList->freezeDataList.list);
    p_accumFreezeList->freezeDataList.capacity = 0;
    p_accumFreezeList->freezeDataList.count = 0;
    p_accumFreezeList->freezeDataList.idx = 0;
    p_accumFreezeList->freezeDataList.free = NULL;
}

/*******************************************************
* 函数名称: accum_freeJsonConfig
* ---------------------------------------------------
* 功能描述: 释放配置文件解析结果指针
* ---------------------------------------------------
* 输入参数: p_EnergyConfig - 配置文件解析结果指针
* ---------------------------------------------------
* 输出参数: 无
* ---------------------------------------------------
* 返回值: 无
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static void accum_freeJsonConfig(accumEnergyConfig_s *p_EnergyConfig)
{
    if (p_EnergyConfig == NULL)
    {
        return;
    }

    ACCUM_FREE_DATA(p_EnergyConfig->accumItemList.list);
    p_EnergyConfig->accumItemList.capacity = 0;
    p_EnergyConfig->accumItemList.count = 0;
    p_EnergyConfig->accumItemList.idx = 0;
    p_EnergyConfig->accumItemList.free = NULL;

    ACCUM_FREE_DATA(p_EnergyConfig->freezeItemList.list);
    p_EnergyConfig->freezeItemList.capacity = 0;
    p_EnergyConfig->freezeItemList.count = 0;
    p_EnergyConfig->freezeItemList.idx = 0;
    p_EnergyConfig->freezeItemList.free = NULL;
}

/*******************************************************
* 函数名称: accum_freeEnergyList
* ---------------------------------------------------
* 功能描述: 释放总的电能数据项列表指针
* ---------------------------------------------------
* 输入参数: p_accumEnergyList - 总的电能数据项列表指针
* ---------------------------------------------------
* 输出参数: 无
* ---------------------------------------------------
* 返回值: 无
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static void accum_freeEnergyList(accumEnergyList_s *p_accumEnergyList)
{
    if (p_accumEnergyList == NULL)
    {
        return;
    }

    int i = 0;
    for (i = 0; i < p_accumEnergyList->phaseDataList.capacity; i++)
    {
        accum_freeFreezeList(&p_accumEnergyList->phaseDataList.list[i]);
    }

    ACCUM_FREE_DATA(p_accumEnergyList->phaseDataList.list);
    p_accumEnergyList->phaseDataList.capacity = 0;
    p_accumEnergyList->phaseDataList.count = 0;
    p_accumEnergyList->phaseDataList.idx = 0;
    p_accumEnergyList->phaseDataList.free = NULL;

    accum_freeJsonConfig(p_accumEnergyList->pEnergyConfig);
    ACCUM_FREE_DATA(p_accumEnergyList->pEnergyConfig);
}

/*******************************************************
* 函数名称: accum_printJson
* ---------------------------------------------------
* 功能描述: 打印配置文件解析结果
* ---------------------------------------------------
* 输入参数: p_accumEnergyConfig - 配置文件解析结果指针
* ---------------------------------------------------
* 输出参数: 无
* ---------------------------------------------------
* 返回值: 无
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static void accum_printJson(accumEnergyConfig_s *p_accumEnergyConfig)
{
    if (p_accumEnergyConfig == NULL)
    {
        printf("JsonConfig Is NULL\n");
        return;
    }

    printf("acqPeriod = %d\n", p_accumEnergyConfig->acqPeriod);
    printf("resultType = %d\n", p_accumEnergyConfig->resultType);
    printf("startRealDbNo = %d\n", p_accumEnergyConfig->startRealDbNo);

    printf("accumItemList:\n");

    int i = 0;
    for (i = 0; i < p_accumEnergyConfig->accumItemList.capacity; i++)
    {
        printf("\taccumItemList[%d].realDbNo = %d\n", i, p_accumEnergyConfig->accumItemList.list[i].realDbNo);
        printf("\taccumItemList[%d].description = %s\n", i, p_accumEnergyConfig->accumItemList.list[i].description);
    }

    printf("freezeItemList:\n");
    for (i = 0; i < p_accumEnergyConfig->freezeItemList.capacity; i++)
    {
        printf("\tfreezeItemList[%d].type = %d\n", i, p_accumEnergyConfig->freezeItemList.list[i].type);
        printf("\tfreezeItemList[%d].depth = %d\n", i, p_accumEnergyConfig->freezeItemList.list[i].depth);
    }
}

/*******************************************************
* 函数名称: accum_readconfig
* ---------------------------------------------------
* 功能描述: 根据配置文件解析结果, 初始化各冻结列表
* ---------------------------------------------------
* 输入参数: link - 链路配置指针
* ---------------------------------------------------
* 输出参数: p_accumEnergyList - 总的电能数据项列表指针
* ---------------------------------------------------
* 返回值: 0 - 初始化成功;
*       -1 - 初始化失败
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static int accum_readconfig(accumEnergyConfig_s *pConfig, accumEnergyList_s *p_accumEnergyList)
{
    if (pConfig == NULL || p_accumEnergyList == NULL)
    {
        return -1;
    }

    p_accumEnergyList->pEnergyConfig = pConfig;

    switch (p_accumEnergyList->pEnergyConfig->resultType)
    {
        case e_accumulate_result_total:
        {
            p_accumEnergyList->phaseDataList.list = calloc(1, sizeof(accumFreezeList_s));
            if (p_accumEnergyList->phaseDataList.list == NULL)
            {
                accum_freeJsonConfig(p_accumEnergyList->pEnergyConfig);
                ACCUM_FREE_DATA(p_accumEnergyList->pEnergyConfig);
                return -1;
            }

            p_accumEnergyList->phaseDataList.capacity = 1;
            p_accumEnergyList->phaseDataList.count = 1;
        }

            break;
        case e_accumulate_result_divide:
        {
            p_accumEnergyList->phaseDataList.list = calloc(ACCUM_PHASE_COUNT, sizeof(accumFreezeList_s));
            if (p_accumEnergyList->phaseDataList.list == NULL)
            {
                accum_freeJsonConfig(p_accumEnergyList->pEnergyConfig);
                ACCUM_FREE_DATA(p_accumEnergyList->pEnergyConfig);
                return -1;
            }

            p_accumEnergyList->phaseDataList.capacity = ACCUM_PHASE_COUNT;
            p_accumEnergyList->phaseDataList.count = ACCUM_PHASE_COUNT;
        }

            break;
        default:
            accum_freeJsonConfig(p_accumEnergyList->pEnergyConfig);
            ACCUM_FREE_DATA(p_accumEnergyList->pEnergyConfig);
            return -1;
    }

    int i = 0;     //分相数据的索引
    int j = 0;     //每个分相数据的冻结类型索引
    int k = 0;     //每个冻结类型的数据索引
    int count = 0;
    int realNo = p_accumEnergyList->pEnergyConfig->startRealDbNo;
    int devno = 0;
    int regno = 0;
    char desc[1024] = { 0 };
    char last[1024] = { 0 };
    char dayOrMonth[10] = { 0 };
    for (i = 0; i < p_accumEnergyList->phaseDataList.capacity; i++)
    {
        p_accumEnergyList->phaseDataList.list[i].freezeDataList.list = calloc(e_accumulate_type_count, sizeof(accumDataList_s));
        if (p_accumEnergyList->phaseDataList.list[i].freezeDataList.list == NULL)
        {
            accum_freeEnergyList(p_accumEnergyList);
            return -1;
        }

        p_accumEnergyList->phaseDataList.list[i].freezeDataList.capacity = e_accumulate_type_count;
        p_accumEnergyList->phaseDataList.list[i].freezeDataList.count = e_accumulate_type_count;
        p_accumEnergyList->phaseDataList.list[i].freezeDataList.idx = 0;
        p_accumEnergyList->phaseDataList.list[i].freezeDataList.free = NULL;

        p_accumEnergyList->phaseDataList.list[i].srcDbNo = p_accumEnergyList->pEnergyConfig->accumItemList.list[i].realDbNo;
        strcpy(p_accumEnergyList->phaseDataList.list[i].description, p_accumEnergyList->pEnergyConfig->accumItemList.list[i].description);

        switch (p_accumEnergyList->pEnergyConfig->resultType)
        {
            case e_accumulate_result_total:
                strcpy(desc, "三相");
                break;
            case e_accumulate_result_divide:
                switch (i)
                {
                    case e_accum_phase_A:
                        strcpy(desc, "A相");
                        break;
                    case e_accum_phase_B:
                        strcpy(desc, "B相");
                        break;
                    case e_accum_phase_C:
                        strcpy(desc, "C相");
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }

        for (j = 0; j < p_accumEnergyList->phaseDataList.list[i].freezeDataList.capacity; j++)
        {
            count = p_accumEnergyList->pEnergyConfig->freezeItemList.list[j].depth + 1;
            p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.list = calloc(count, sizeof(accumDataItem_s));
            if (p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.list == NULL)
            {
                accum_freeEnergyList(p_accumEnergyList);
                return -1;
            }

            p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.capacity = count;
            p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.count = 1;     //至少有1个当前{日/月/年/周}的数值
            p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.idx = 0;
            p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.free = NULL;
            p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].accumType = j;

            switch (j)
            {
                case e_accumulate_day:
                    strcpy(dayOrMonth, "日");
                    break;
                case e_accumulate_month:
                    strcpy(dayOrMonth, "月");
                    break;
                case e_accumulate_year:
                    strcpy(dayOrMonth, "年");
                    break;
                case e_accumulate_week:
                    strcpy(dayOrMonth, "周");
                    break;
                default:
                    break;
            }

            for (k = 0, regno = 0; k < p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.capacity; k++)
            {
                p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.list[k].realDbNo = realNo;
                p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.list[k].linkNo = 3;
                p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.list[k].devNo = devno;
                p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.list[k].regNo = regno;
                regno++;
                realNo++;

                strcpy(p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.list[k].description, desc);
                if (k == 0)
                {
                    strcat(p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.list[k].description, "当");
                }
                else
                {
                    strcat(p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.list[k].description, "上");
                    snprintf(last, sizeof(last) - 1, "%d次", k);
                    strcat(p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.list[k].description, last);
                }

                strcat(p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.list[k].description, dayOrMonth);
                strcat(p_accumEnergyList->phaseDataList.list[i].freezeDataList.list[j].dataList.list[k].description, "冻结电能");
            }

            devno++;
        }
    }

    return 0;
}

/*******************************************************
* 函数名称: accum_printDataList
* ---------------------------------------------------
* 功能描述: 打印1个数据项列表
* ---------------------------------------------------
* 输入参数: p_dataList - 数据项列表指针
*         depth - 当前列表所处的打印深度
* ---------------------------------------------------
* 输出参数: 无
* ---------------------------------------------------
* 返回值: 无
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static void accum_printDataList(accumDataList_s *p_dataList, int depth)
{
    char tabs[10] = { 0 };
    int i = 0;
    for (i = 0; i < depth; i++)
    {
        strcat(tabs, "\t");
    }

    if (p_dataList == NULL)
    {
        printf("%sNULL\n", tabs);
        return;
    }

    printf("%stype: %d\n", tabs, p_dataList->accumType);
    printf("%sDataList:\n", tabs);
    for (i = 0; i < p_dataList->dataList.capacity; i++)
    {
        printf("\t%srealDbNo: %d\n", tabs, p_dataList->dataList.list[i].realDbNo);
        printf("\t%sdescription: %s\n", tabs, p_dataList->dataList.list[i].description);
        printf("\t%sfreezeValue: %f\n", tabs, p_dataList->dataList.list[i].freezeValue);
        printf("\t%sfreezeTime: %04d-%02d-%02d %02d:%02d:%02d %d\n", tabs,
                p_dataList->dataList.list[i].freezeTime.Year,
                p_dataList->dataList.list[i].freezeTime.Month,
                p_dataList->dataList.list[i].freezeTime.Day,
                p_dataList->dataList.list[i].freezeTime.Hour,
                p_dataList->dataList.list[i].freezeTime.Minute,
                p_dataList->dataList.list[i].freezeTime.Second,
                p_dataList->dataList.list[i].freezeTime.weekday);
    }
}

/*******************************************************
* 函数名称: accum_printFreezeList
* ---------------------------------------------------
* 功能描述: 打印1个单相数据项列表
* ---------------------------------------------------
* 输入参数: p_freezeList - 数据项列表指针
*         depth - 当前列表所处的打印深度
* ---------------------------------------------------
* 输出参数: 无
* ---------------------------------------------------
* 返回值: 无
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static void accum_printFreezeList(accumFreezeList_s *p_freezeList, int depth)
{
    char tabs[10] = { 0 };
    int i = 0;
    for (i = 0; i < depth; i++)
    {
        strcat(tabs, "\t");
    }

    if (p_freezeList == NULL)
    {
        printf("%sNULL\n", tabs);
        return;
    }

    printf("%ssrcDbNo: %d\n", tabs, p_freezeList->srcDbNo);
    printf("%sdescription: %s\n", tabs, p_freezeList->description);
    printf("%sfreezeDataList:\n", tabs);
    for (i = 0; i < p_freezeList->freezeDataList.capacity; i++)
    {
        accum_printDataList(&p_freezeList->freezeDataList.list[i], depth + 1);
    }
}

/*******************************************************
* 函数名称: accum_printEnergyList
* ---------------------------------------------------
* 功能描述: 打印总的电能数据列表
* ---------------------------------------------------
* 输入参数: p_energyList - 总的电能数据列表指针
*         depth - 当前列表所处的打印深度
* ---------------------------------------------------
* 输出参数: 无
* ---------------------------------------------------
* 返回值: 无
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static void accum_printEnergyList(accumEnergyList_s *p_energyList, int depth)
{
    char tabs[10] = { 0 };
    int i = 0;
    for (i = 0; i < depth; i++)
    {
        strcat(tabs, "\t");
    }

    if (p_energyList == NULL)
    {
        printf("%sNULL\n", tabs);
        return;
    }

    accum_printJson(p_energyList->pEnergyConfig);
    printf("%sphaseDataList:\n", tabs);
    for (i = 0; i < p_energyList->phaseDataList.capacity; i++)
    {
        accum_printFreezeList(&p_energyList->phaseDataList.list[i], depth + 1);
    }
}

u16 accum_saveDataItem(accumDataItem_s *p_dataItem, u8 *pBuf, u16 bufMaxSize)
{
    if (p_dataItem == NULL)
    {
        return 0;
    }

    u16 offset = 0;

    memcpy(pBuf + offset, &p_dataItem->realDbNo, sizeof(p_dataItem->realDbNo));
    offset += sizeof(p_dataItem->realDbNo);//2 bytes

    memcpy(pBuf + offset, &p_dataItem->freezeValue, sizeof(p_dataItem->freezeValue));
    offset += sizeof(p_dataItem->freezeValue);//4 bytes

    memcpy(pBuf + offset, &p_dataItem->freezeTime.Year, sizeof(p_dataItem->freezeTime.Year));
    offset += sizeof(p_dataItem->freezeTime.Year);//2 bytes

    memcpy(pBuf + offset, &p_dataItem->freezeTime.Month, sizeof(p_dataItem->freezeTime.Month));
    offset += sizeof(p_dataItem->freezeTime.Month);//1 byte

    memcpy(pBuf + offset, &p_dataItem->freezeTime.Day, sizeof(p_dataItem->freezeTime.Day));
    offset += sizeof(p_dataItem->freezeTime.Day);//1 byte

    memcpy(pBuf + offset, &p_dataItem->freezeTime.Hour, sizeof(p_dataItem->freezeTime.Hour));
    offset += sizeof(p_dataItem->freezeTime.Hour);//1 byte

    memcpy(pBuf + offset, &p_dataItem->freezeTime.Minute, sizeof(p_dataItem->freezeTime.Minute));
    offset += sizeof(p_dataItem->freezeTime.Minute);//1 byte

    memcpy(pBuf + offset, &p_dataItem->freezeTime.Second, sizeof(p_dataItem->freezeTime.Second));
    offset += sizeof(p_dataItem->freezeTime.Second);//1 byte

    memcpy(pBuf + offset, &p_dataItem->freezeTime.weekday, sizeof(p_dataItem->freezeTime.weekday));
    offset += sizeof(p_dataItem->freezeTime.weekday);//1 byte

    return offset;
}

u16 accum_saveDataList(accumDataList_s *p_dataList, u8 *pBuf, u16 bufMaxSize)
{
    if (p_dataList == NULL)
    {
        return 0;
    }

    u16 offset = 0;

    u16 accumType = (u16)p_dataList->accumType;
    memcpy(pBuf + offset, &accumType, sizeof(accumType));
    offset += sizeof(accumType);//2 bytes

    u16 dataCount = (u16)p_dataList->dataList.capacity;
    memcpy(pBuf + offset, &dataCount, sizeof(dataCount));
    offset += sizeof(dataCount);//2 bytes

    int i = 0;
    for (i = 0; i < dataCount && bufMaxSize >= offset; i++)
    {
        offset += accum_saveDataItem(&p_dataList->dataList.list[i], pBuf + offset, bufMaxSize - offset);
    }

    return offset;
}

u16 accum_saveFreezeList(accumFreezeList_s *p_freezeList, u8 *pBuf, u16 bufMaxSize)
{
    if (p_freezeList == NULL)
    {
        return 0;
    }

    u16 offset = 0;

    u16 srcDbNo = p_freezeList->srcDbNo;
	memcpy(pBuf + offset, &srcDbNo, sizeof(srcDbNo));
	offset += sizeof(srcDbNo);//2 bytes

	u16 dataListCount = p_freezeList->freezeDataList.capacity;
	memcpy(pBuf + offset, &dataListCount, sizeof(dataListCount));
	offset += sizeof(dataListCount);//2 bytes

    int i = 0;
    for (i = 0; i < dataListCount && bufMaxSize >= offset; i++)
    {
    	offset += accum_saveDataList(&p_freezeList->freezeDataList.list[i], pBuf + offset, bufMaxSize - offset);
    }

    return offset;
}

/*******************************************************
* 函数名称: accum_saveData
* ---------------------------------------------------
* 功能描述: 保存总的数据列表
* ---------------------------------------------------
* 输入参数: link - 链路配置指针
*         p_freezeList - 总的数据列表指针
* ---------------------------------------------------
* 输出参数: 无
* ---------------------------------------------------
* 返回值: 无
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static int accum_saveData(const char *configFilename, accumEnergyList_s *p_accumEnergyList, u8 *pBuf, u16 bufMaxSize)
{
    if (p_accumEnergyList == NULL)
    {
        return 0;
    }

    u16 offset = 0;
    int i = 0;

    //
    //保存配置文件的md5校验码
    //
    MD5File(configFilename, p_accumEnergyList->jsonMd5);
    memcpy(pBuf + offset, p_accumEnergyList->jsonMd5, sizeof(p_accumEnergyList->jsonMd5));
	offset += sizeof(p_accumEnergyList->jsonMd5);//16 bytes

    //
    //保存数据列表
    //
    u16 phaseCount = p_accumEnergyList->phaseDataList.capacity;
    memcpy(pBuf + offset, &phaseCount, sizeof(phaseCount));
	offset += sizeof(phaseCount);//2 bytes

    for (i = 0; i < phaseCount && bufMaxSize >= offset; i++)
    {
        offset += accum_saveFreezeList(&p_accumEnergyList->phaseDataList.list[i], pBuf + offset, bufMaxSize - offset);
    }

    return offset;
}

/*******************************************************
* 函数名称: accum_readDataItem
* ---------------------------------------------------
* 功能描述: 读取1个数据项
* ---------------------------------------------------
* 输入参数: item - json指针
*         p_dataItem - 数据项指针
* ---------------------------------------------------
* 输出参数: 无
* ---------------------------------------------------
* 返回值: 无
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static u16 accum_readDataItem(accumDataItem_s *p_dataItem, u8 *pBuf, u16 bufSize)
{
    if (p_dataItem == NULL || pBuf == NULL)
    {
        return 0;
    }

    u16 offset = 0;

    memcpy(&p_dataItem->realDbNo, pBuf + offset, sizeof(p_dataItem->realDbNo));
    offset += sizeof(p_dataItem->realDbNo);    //2 bytes

//    p_dataItem->linkNo = json_integer_value(json_object_get(item, "linkNo"));
//    p_dataItem->devNo = json_integer_value(json_object_get(item, "devNo"));
//    p_dataItem->regNo = json_integer_value(json_object_get(item, "regNo"));

    memcpy(&p_dataItem->freezeValue, pBuf + offset, sizeof(p_dataItem->freezeValue));
    offset += sizeof(p_dataItem->freezeValue);    //4 bytes

    memcpy(&p_dataItem->freezeTime.Year, pBuf + offset, sizeof(p_dataItem->freezeTime.Year));
    offset += sizeof(p_dataItem->freezeTime.Year);    //2 bytes

    memcpy(&p_dataItem->freezeTime.Month, pBuf + offset, sizeof(p_dataItem->freezeTime.Month));
    offset += sizeof(p_dataItem->freezeTime.Month);    //1 bytes

    memcpy(&p_dataItem->freezeTime.Day, pBuf + offset, sizeof(p_dataItem->freezeTime.Day));
    offset += sizeof(p_dataItem->freezeTime.Day);    //1 bytes

    memcpy(&p_dataItem->freezeTime.Hour, pBuf + offset, sizeof(p_dataItem->freezeTime.Hour));
    offset += sizeof(p_dataItem->freezeTime.Hour);    //1 bytes

    memcpy(&p_dataItem->freezeTime.Minute, pBuf + offset, sizeof(p_dataItem->freezeTime.Minute));
    offset += sizeof(p_dataItem->freezeTime.Minute);    //1 bytes

    memcpy(&p_dataItem->freezeTime.Second, pBuf + offset, sizeof(p_dataItem->freezeTime.Second));
    offset += sizeof(p_dataItem->freezeTime.Second);    //1 bytes

    memcpy(&p_dataItem->freezeTime.weekday, pBuf + offset, sizeof(p_dataItem->freezeTime.weekday));
    offset += sizeof(p_dataItem->freezeTime.weekday);    //1 bytes

    return offset;
}

/*******************************************************
* 函数名称: accum_readDataList
* ---------------------------------------------------
* 功能描述: 读取1个数据项列表
* ---------------------------------------------------
* 输入参数: item - json指针
*         p_dataList - 数据项列表指针
* ---------------------------------------------------
* 输出参数: 无
* ---------------------------------------------------
* 返回值: 无
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static u16 accum_readDataList(accumDataList_s *p_dataList, u8 *pBuf, u16 bufSize)
{
    if (pBuf == NULL || p_dataList == NULL)
    {
        return 0;
    }

    u16 offset = 0;

    u16 acumType = 0;
    memcpy(&acumType, pBuf + offset, sizeof(acumType));
    p_dataList->accumType = (accumEnergyType_e) acumType;
    offset += sizeof(acumType);    //2 bytes

    u16 count = 0;
    memcpy(&count, pBuf + offset, sizeof(count));
    offset += sizeof(count);    //2 bytes

    p_dataList->dataList.list = (accumDataItem_s*) calloc(count, sizeof(accumDataItem_s));
    if (p_dataList->dataList.list == NULL)
    {
        p_dataList->dataList.capacity = 0;
        return 0;
    }

    p_dataList->dataList.capacity = count;

    int i = 0;
    for (i = 0; i < count && bufSize >= offset; i++)
    {
        offset += accum_readDataItem(&p_dataList->dataList.list[i], pBuf + offset, bufSize - offset);
    }

    return offset;
}

/*******************************************************
* 函数名称: accum_readFreezeList
* ---------------------------------------------------
* 功能描述: 读取1个单相数据列表
* ---------------------------------------------------
* 输入参数: item - json指针
*         p_freezeList - 单相数据列表指针
* ---------------------------------------------------
* 输出参数: 无
* ---------------------------------------------------
* 返回值: 无
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static u16 accum_readFreezeList(accumFreezeList_s *p_freezeList, u8 *pBuf, u16 bufSize)
{
    if (pBuf == NULL || p_freezeList == NULL)
    {
        return 0;
    }

    u16 offset = 0;

    u16 srcDbNo = 0;
    memcpy(&srcDbNo, pBuf + offset, sizeof(srcDbNo));
    p_freezeList->srcDbNo = srcDbNo;
    offset += sizeof(srcDbNo);

    u16 count = 0;
    memcpy(&count, pBuf + offset, sizeof(count));
    offset += sizeof(count);

    p_freezeList->freezeDataList.list = (accumDataList_s*) calloc(count, sizeof(accumDataList_s));
    if (p_freezeList->freezeDataList.list == NULL)
    {
        p_freezeList->freezeDataList.capacity = 0;
        return offset;
    }

    p_freezeList->freezeDataList.capacity = count;

    int i = 0;
    for (i = 0; i < count && bufSize >= offset; i++)
    {
        offset += accum_readDataList(&p_freezeList->freezeDataList.list[i], pBuf + offset, bufSize - offset);
    }

    return offset;
}

/*******************************************************
* 函数名称: accum_readData
* ---------------------------------------------------
* 功能描述: 读取上次保存的数据文件
* ---------------------------------------------------
* 输入参数: filename - 数据文件名
* ---------------------------------------------------
* 输出参数: p_energyList - 读取结果
* ---------------------------------------------------
* 返回值: 0 - 读取成功;
*      -1 - 读取失败
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static u16 accum_readData(accumEnergyList_s *p_energyList, u8 *pBuf, u16 bufSize)
{
    if (pBuf == NULL || p_energyList == NULL)
    {
        return 0;
    }

    u16 offset = 0;

    memcpy(p_energyList->jsonMd5, pBuf + offset, sizeof(p_energyList->jsonMd5));
    offset += sizeof(p_energyList->jsonMd5);

    u16 count = 0;
    memcpy(&count, pBuf + offset, sizeof(count));
    offset += sizeof(count);

    p_energyList->phaseDataList.list = (accumFreezeList_s*) calloc(count, sizeof(accumFreezeList_s));
    if (p_energyList->phaseDataList.list == NULL)
    {
        p_energyList->phaseDataList.capacity = 0;
        return offset;
    }

    p_energyList->phaseDataList.capacity = count;

    int i = 0;
    for (i = 0; i < count && bufSize >= offset; i++)
    {
        offset += accum_readFreezeList(&p_energyList->phaseDataList.list[i], pBuf + offset, bufSize - offset);
    }

    return offset;
}

/*******************************************************
* 函数名称: accum_readFlash
* ---------------------------------------------------
* 功能描述: 对比上次保存的数据文件中的配置文件的md5校验码和当前的
*         配置文件的md5校验码, 如果二者相同, 则读取上次保存的数据;
*         如果二者不同, 或者没找到上次保存的数据文件, 则重新读取
*         当前的配置.
* ---------------------------------------------------
* 输入参数: link - 链路配置指针
* ---------------------------------------------------
* 输出参数: p_energyList - 读取的结果指针
* ---------------------------------------------------
* 返回值: 0 - 读取成功;
*      -1 - 读取失败
* ---------------------------------------------------
* 补充信息: 无
* 修改日志: 无
*******************************************************/
static int accum_readFlash(const char *configFileName, accumEnergyConfig_s *pConfig, accumEnergyList_s *p_energyList, u8 *pBuf, u16 bufSize)
{
    if (pBuf == NULL || p_energyList == NULL)
    {
        return -1;
    }

    //
    //如果以前保存过数据, 读取其保存的配置文件的md5码和数据
    //
    u16 offset = accum_readData(p_energyList, pBuf, bufSize);
    if (offset == 0)
    {
        return -1;
    }

    //
    //读取配置文件的md5码
    //
    u8 md5[16] = { 0 };
    MD5File(configFileName, md5);

    //
    //如果md5码不匹配, 则返回-1, 且释放之前的数据所占用的内存
    //
    if (strncmp((const char*) p_energyList->jsonMd5, (const char*) md5, sizeof(md5)) != 0)
    {
        accum_freeEnergyList(p_energyList);
        return -1;
    }

    p_energyList->pEnergyConfig = pConfig;

    return 0;
}

void testAccum(void)
{
    const char configFileName[] = "/home/floyd/repo/mytesting/Debug/accumConfig.json";
    accumEnergyConfig_s* pConfig = parse_accumulate_config(configFileName);
    if (pConfig == NULL)
    {
        DEBUG_TIME_LINE("parse config error");
        return;
    }

    accumEnergyList_s energyListSave;
    accum_readconfig(pConfig, &energyListSave);

    accum_printEnergyList(&energyListSave, 0);

    u8 buf[81920] = { 0 };
    u16 bufSize = (u16)sizeof(buf);
    u16 offset = accum_saveData(configFileName, &energyListSave, buf, bufSize);

    DEBUG_TIME_LINE("offset = %d", offset);

    DEBUG_BUFF_FORMAT(buf, offset, "buf:--->>> ");

    accumEnergyList_s energyListRead;
    if (accum_readFlash(configFileName, pConfig, &energyListRead, buf, offset) == 0)
    {
        DEBUG_TIME_LINE("after read data:----------------------");
        accum_printEnergyList(&energyListRead, 0);
    }
}
