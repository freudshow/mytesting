#include "basedef.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define REAL_DATA_LIST_INIT_SIZE    10  //列表初始大小
#define REAL_DATA_LIST_DELTA_SIZE    5  //列表增量大小

#define REAL_DATA_MAX_DEPTH           128//最大深度

typedef struct dataItemStruct {
    int seq;           //序号
    int devNo;         //设备号
    int linkNo;        //链路号
    int type;          //遥测遥信类型
    int realNo;        //实时库号
    float value;       //实时库值
    float PreVal;      //实时库上次的值
    char modeName[16]; //模型名, 比如"YX_0", "YC_34"
} dataItem_s;

typedef A_LIST_OF(dataItem_s) dataItemList_s;
typedef struct dataListStruct dataList_s;

/**
 * 相同遥测遥信类型, 且实时库连续
 * 的各个数据项列表
 */
typedef struct dataListStruct {
    int seq;                        //序号
    int devNo;                      //设备号
    int type;                       //遥测遥信类型
    dataItemList_s dataItemList;    //实时库列表
    int (*init)(dataList_s *pList, u32 size, int devNo, int linkNo, int type);
    void (*free)(dataList_s *pList);
    int (*append)(dataList_s *pList, int totalCount, dataItem_s *pDataItem);
    int (*getItemByRealNo)(dataList_s *pList, int realNo, dataItem_s *pDataItem);
    int (*getFirst)(dataList_s *pList, dataItem_s *pDataItem);
    int (*getLast)(dataList_s *pList, dataItem_s *pDataItem);
    void (*print)(dataList_s *pList, u32 depth);
} dataList_s;

typedef A_LIST_OF(dataList_s) divDataItemList_s;
typedef struct oneTypeListStruct oneType_s;

/**
 * 一个遥测遥信类型
 */
typedef struct oneTypeListStruct {
    int seq;                        //序号
    int devNo;                      //设备号
    int type;                       //遥测遥信类型
    int totalCount;                 //总个数
    divDataItemList_s divDataList;  //不连续的实时库列表
    int (*append)(oneType_s *pList, dataItem_s *pDataItem);
    void (*print)(oneType_s *pList, u32 depth);
    void (*free)(oneType_s *pList);
    int (*getItemByRealNo)(oneType_s *pOneType, int realNo, dataItem_s *pDataItem);
} oneType_s;

typedef A_LIST_OF(oneType_s) typeList_s;
typedef struct oneDeviceStruct oneDevice_s;

/**
 * 一个设备
 */
typedef struct oneDeviceStruct {
    int seq;                //序号
    int devNo;              //设备号
    typeList_s typeList;    //遥测遥信类型列表
    void (*free)(oneDevice_s *pList);
    int (*append)(oneDevice_s *pList, dataItem_s *pDataItem);
    void (*print)(oneDevice_s *pList, u32 depth);
    int (*getItemByRealNo)(oneDevice_s *pOneDev, int realNo, dataItem_s *pDataItem);
} oneDevice_s;

typedef A_LIST_OF(oneDevice_s) devList_s;    //设备列表, 总的实时库表

#pragma pack(push)
#pragma pack(1)

struct LtuDevHead     //ltudevlib.dat文件头
{
    u8 YcLimiteEnable;
    int TotalYcCount;
    int TotalYxCount;
    int TotalYkCount;
    int TotalDDCount;
    int DataItemCount;
};

struct RealdatabasePara                 //ltudevlib.dat文件体
{
    int RealDevOrderNum;                 //实时库序号
    int ProtolNum;                       //规约号
    short linkNo;                        //链路号
    short devNo;                         //设备号
    short RegNo;                         //寄存器号
    u8 type;                           //0-遥信   1-遥测   2-遥控  3-电度  4-参数
    float Ratio;                         //系数
    float DeadSet;                       //死区
    u8 LessOrMore;                     //0 不进行判断    1 大于死区值时
    u8 GenerateVirtueYx;               //生成虚拟遥信   1 生成虚拟遥信    0  不生成
    int TypeProperty;                    //遥信、遥控的属性的偏移量   根据偏移量，ga_sysparam[TypeProperty]的值，找到 遥信是否取反 及遥控的参数
    float NewYcOverLimite;               //2021-12-24 新增遥测越限  与死区值分开
    float YcLowLimite;                   //2022-4-12 新增遥测越下限
    u8 rev[16];                          //16个保留字节
};

#pragma pack(pop)

static void freeDataList(dataList_s *pList)
{
    assert(pList != NULL && pList->dataItemList.list != NULL);

    free(pList->dataItemList.list);
    pList->dataItemList.list = NULL;
    pList->dataItemList.capacity = 0;
    pList->dataItemList.count = 0;
    pList->dataItemList.idx = 0;

    pList->devNo = -1;
}

static int appendDataItemToDataList(dataList_s *pList, int totalCount, dataItem_s *pDataItem)
{
    assert(pList != NULL && pDataItem != NULL);

    if (pList->dataItemList.list == NULL || pList->dataItemList.capacity == 0)
    {
        INIT_LIST(pList->dataItemList, dataItem_s, REAL_DATA_LIST_INIT_SIZE, free);
    }

    assert(pList->dataItemList.list != NULL);

    if (pList->dataItemList.count == pList->dataItemList.capacity)
    {
        dataItem_s *p = (dataItem_s*) calloc((pList->dataItemList.capacity + REAL_DATA_LIST_DELTA_SIZE), sizeof(dataItem_s));
        if (p == NULL)
        {
            return -1;
        }

        memcpy(p, pList->dataItemList.list, pList->dataItemList.count * sizeof(dataItem_s));

        if (pList->dataItemList.free != NULL)
        {
            pList->dataItemList.free(pList->dataItemList.list);
        }
        else
        {
            free(pList->dataItemList.list);
        }

        pList->dataItemList.list = p;
        pList->dataItemList.capacity += REAL_DATA_LIST_DELTA_SIZE;
    }

    int idx = -1;

    //保证列表有序且连续
    if (0 == pList->dataItemList.count)
    {
        pList->devNo = pDataItem->devNo;
        pList->type = pDataItem->type;
        idx = 0;
    }
    else if (pList->dataItemList.list[pList->dataItemList.count - 1].realNo == (pDataItem->realNo - 1) &&
            pList->dataItemList.list[pList->dataItemList.count - 1].type == pDataItem->type)
    {
        idx = pList->dataItemList.count;
    }

    if (idx < 0)
    {
        return -1;
    }

    switch (pDataItem->type)
    {
        case 0:
            snprintf(pDataItem->modeName, sizeof(pDataItem->modeName) - 1, "YX_%d", totalCount);
            break;
        case 1:
            snprintf(pDataItem->modeName, sizeof(pDataItem->modeName) - 1, "YC_%d", totalCount);
            break;
        case 2:
            snprintf(pDataItem->modeName, sizeof(pDataItem->modeName) - 1, "YK_%d", totalCount);
            break;
        case 3:
            snprintf(pDataItem->modeName, sizeof(pDataItem->modeName) - 1, "DD_%d", totalCount);
            break;
        default:
            snprintf(pDataItem->modeName, sizeof(pDataItem->modeName) - 1, "ERROR_TYPE");
            break;
    }

    pDataItem->seq = idx;
    memcpy(&pList->dataItemList.list[idx], pDataItem, sizeof(dataItem_s));
    pList->dataItemList.count++;

    return 0;
}

static int getItemByRealNo(dataList_s *pList, int realNo, dataItem_s *pDataItem)
{
    assert(pList != NULL && pDataItem != NULL && pList->dataItemList.list != NULL && pList->dataItemList.count > 0 && pList->dataItemList.capacity > 0);

    int low = 0;
    int high = pList->dataItemList.count - 1;
    int mid = 0;
    while (low <= high)
    {
        mid = low + (high - low) / 2;
        if (pList->dataItemList.list[mid].realNo == realNo)
        {
            memcpy(pDataItem, &pList->dataItemList.list[mid], sizeof(dataItem_s));
            return mid;
        }
        else if (pList->dataItemList.list[mid].realNo < realNo)
        {
            low = mid + 1;
        }
        else if (pList->dataItemList.list[mid].realNo > realNo)
        {
            high = mid - 1;
        }
    }

    pDataItem->realNo = -1;
    pDataItem->value = -1;
    pDataItem->devNo = -1;
    pDataItem->linkNo = -1;
    pDataItem->type = -1;
    strncpy(pDataItem->modeName, "NULL", sizeof(pDataItem->modeName) - 1);
    return -1;
}

static int getItemByIdx(dataList_s *pList, u32 idx, dataItem_s *pDataItem)
{
    assert(pList != NULL && pDataItem != NULL && pList->dataItemList.list != NULL && pList->dataItemList.count != 0 && pList->dataItemList.capacity > 0);

    if (idx >= pList->dataItemList.count)
    {
        return -1;
    }

    memcpy(pDataItem, &pList->dataItemList.list[idx], sizeof(dataItem_s));
    return 0;
}

static int getFirstDataItem(dataList_s *pList, dataItem_s *pDataItem)
{
    return getItemByIdx(pList, 0, pDataItem);
}

static int getLastDataItem(dataList_s *pList, dataItem_s *pDataItem)
{
    return getItemByIdx(pList, pList->dataItemList.count - 1, pDataItem);
}

static void printDataItem(dataItem_s *pItem, u32 depth)
{
    int i = 0;
    char depthstr[REAL_DATA_MAX_DEPTH + 1] = { 0 };
    for (i = 0; i < depth; i++)
    {
        strncat(depthstr, "\t", REAL_DATA_MAX_DEPTH);
    }

    printf("%s-------item-------\n", depthstr);
    if (pItem == NULL)
    {
        printf("%spItem is NULL\n", depthstr);
        return;
    }

    printf("%sseq: %d\n", depthstr, pItem->seq);
    printf("%sdevNo: %d\n", depthstr, pItem->devNo);
    printf("%slinkNo: %d\n", depthstr, pItem->linkNo);
    printf("%stype: %d\n", depthstr, pItem->type);
    printf("%srealNo: %d\n", depthstr, pItem->realNo);
    printf("%svalue: %f\n", depthstr, pItem->value);
    printf("%sPreVal: %f\n", depthstr, pItem->PreVal);
    printf("%smodeName: %s\n", depthstr, pItem->modeName);
}

static void printDataList(dataList_s *pList, u32 depth)
{
    int i = 0;
    char depthstr[REAL_DATA_MAX_DEPTH + 1] = { 0 };
    for (i = 0; i < depth; i++)
    {
        strncat(depthstr, "\t", REAL_DATA_MAX_DEPTH);
    }

    printf("%s------dataItemList--------\n", depthstr);
    if (pList == NULL)
    {
        printf("%spList is NULL\n", depthstr);
        return;
    }

    printf("%sseq: %d\n", depthstr, pList->seq);
    printf("%sdevNo: %d, type: %d\n", depthstr, pList->devNo, pList->type);
    printf("%sList: %p, count: %d, capacity: %d\n", depthstr, pList->dataItemList.list, pList->dataItemList.count, pList->dataItemList.capacity);

    for (i = 0; i < pList->dataItemList.count; i++)
    {
        printDataItem(&pList->dataItemList.list[i], depth + 1);
    }
}

static int initDataList(dataList_s *pList, u32 size, int devNo, int linkNo, int type)
{
    assert(pList != NULL && size > 0);

    INIT_LIST(pList->dataItemList, dataItem_s, size, free);

    if (pList->dataItemList.list == NULL)
    {
        return -1;
    }

    pList->devNo = devNo;
    pList->type = type;

    pList->append = appendDataItemToDataList;
    pList->getItemByRealNo = getItemByRealNo;
    pList->getFirst = getFirstDataItem;
    pList->getLast = getLastDataItem;
    pList->print = printDataList;
    pList->free = freeDataList;
    pList->init = initDataList;

    return 0;
}

static void freeOneType(oneType_s *pOneType)
{
    assert(pOneType != NULL && pOneType->divDataList.list != NULL && pOneType->divDataList.capacity > 0);

    int i = 0;
    for (i = 0; i < pOneType->divDataList.capacity; i++)
    {
        pOneType->divDataList.list[i].free(&pOneType->divDataList.list[i]);
    }

    free(pOneType->divDataList.list);
    pOneType->divDataList.list = NULL;
    pOneType->divDataList.capacity = 0;
    pOneType->divDataList.count = 0;

    pOneType->devNo = -1;
    pOneType->type = -1;
}

static int appendDataItemToOneType(oneType_s *pList, dataItem_s *pDataItem)
{
    assert(pList != NULL && pDataItem != NULL && pList->append != NULL && pList->divDataList.list != NULL && pList->divDataList.capacity > 0);

    if (pList->divDataList.count == 0)
    {
        pList->divDataList.list[0].seq = 0;
        pList->devNo = pDataItem->devNo;
        pList->type = pDataItem->type;
        pList->divDataList.count++;
    }

    if (pList->type != pDataItem->type)
    {
        return -1;
    }

    int i = 0;
    for (i = 0; i < pList->divDataList.count; i++)
    {
        if (pList->divDataList.list[i].append(&pList->divDataList.list[i], pList->totalCount, pDataItem) == 0)
        {
            pList->totalCount++;
            return 0;
        }
    }

    if (pList->divDataList.count == pList->divDataList.capacity)
    {
        dataList_s *p = (dataList_s*) calloc((pList->divDataList.capacity + REAL_DATA_LIST_DELTA_SIZE), sizeof(dataList_s));
        if (p == NULL)
        {
            return -1;
        }

        for (i = 0; i < (pList->divDataList.capacity + REAL_DATA_LIST_DELTA_SIZE); i++)
        {
            initDataList(p + i, REAL_DATA_LIST_INIT_SIZE, -1, -1, -1);
        }

        memcpy(p, pList->divDataList.list, pList->divDataList.count * sizeof(dataList_s));
        free(pList->divDataList.list);
        pList->divDataList.list = p;
        pList->divDataList.capacity += REAL_DATA_LIST_DELTA_SIZE;
    }

    u32 idx = pList->divDataList.count;
    if (pList->divDataList.list[idx].append(&pList->divDataList.list[idx], pList->totalCount, pDataItem) == 0)
    {
        pList->divDataList.list[idx].seq = idx;
        pList->divDataList.count++;
        pList->divDataList.list[idx].devNo = pDataItem->devNo;
        pList->divDataList.list[idx].type = pDataItem->type;
        pList->totalCount++;
        return 0;
    }

    return -1;
}

static int getItemByRealNoInOneType(oneType_s *pOneType, int realNo, dataItem_s *pDataItem)
{
    assert(pOneType != NULL && pDataItem != NULL && pOneType->divDataList.list != NULL && pOneType->divDataList.capacity > 0);

    int i = 0;
    for (i = 0; i < pOneType->divDataList.count; i++)
    {
        if (pOneType->divDataList.list[i].getItemByRealNo(&pOneType->divDataList.list[i], realNo, pDataItem) >= 0)
        {
            return 0;
        }
    }

    return -1;
}

static void printOneType(oneType_s *pOneType, u32 depth)
{
    int i = 0;
    char depthstr[REAL_DATA_MAX_DEPTH + 1] = { 0 };
    for (i = 0; i < depth; i++)
    {
        strncat(depthstr, "\t", REAL_DATA_MAX_DEPTH);
    }

    printf("%s----------OneType------------\n", depthstr);

    if (pOneType == NULL)
    {
        printf("%spOneType is NULL\n", depthstr);
        return;
    }

    printf("%sseq: %d\n", depthstr, pOneType->seq);
    printf("%sdevNo: %d\n", depthstr, pOneType->devNo);
    printf("%stype: %d\n", depthstr, pOneType->type);
    printf("%scount: %d\n", depthstr, pOneType->divDataList.count);
    printf("%scapacity: %d\n", depthstr, pOneType->divDataList.capacity);

    for (i = 0; i < pOneType->divDataList.count; i++)
    {
        pOneType->divDataList.list[i].print(&pOneType->divDataList.list[i], depth + 1);
    }
}

static int initOneType(oneType_s *pOneType, u32 size, int devNo, int linkNo, int type)
{
    if (pOneType == NULL)
    {
        return -1;
    }

    INIT_LIST(pOneType->divDataList, dataList_s, size, NULL);

    int i = 0;
    for (i = 0; i < pOneType->divDataList.capacity; i++)
    {
        initDataList(&pOneType->divDataList.list[i], size, devNo, linkNo, type);
    }

    pOneType->free = freeOneType;
    pOneType->append = appendDataItemToOneType;
    pOneType->print = printOneType;
    pOneType->getItemByRealNo = getItemByRealNoInOneType;

    return 0;
}

static void freeOneDev(oneDevice_s *pOneType)
{
    assert(pOneType != NULL && pOneType->typeList.list != NULL && pOneType->typeList.capacity > 0);

    int i = 0;
    for (i = 0; i < pOneType->typeList.capacity; i++)
    {
        pOneType->typeList.list[i].free(&pOneType->typeList.list[i]);
    }

    free(pOneType->typeList.list);
    pOneType->typeList.list = NULL;
    pOneType->typeList.count = 0;
    pOneType->typeList.capacity = 0;

    pOneType->devNo = -1;
}

static int addDataItemToOneDev(oneDevice_s *pOneDev, dataItem_s *pItem)
{
    assert(pOneDev != NULL && pItem != NULL && pOneDev->typeList.list != NULL && pOneDev->typeList.capacity > 0);

    if (pOneDev->typeList.count == 0)
    {
        if (pOneDev->typeList.list[0].append(&pOneDev->typeList.list[0], pItem) == 0)
        {
            pOneDev->devNo = pItem->devNo;
            pOneDev->typeList.list[0].seq = 0;
            pOneDev->typeList.list[0].devNo = pItem->devNo;
            pOneDev->typeList.list[0].type = pItem->type;
            pOneDev->typeList.count++;
            return 0;
        }
        else
        {
            return -1;
        }
    }

    int i = 0;
    for (i = 0; i < pOneDev->typeList.count; i++)
    {
        if (pOneDev->typeList.list[i].append(&pOneDev->typeList.list[i], pItem) == 0)
        {
            return 0;
        }
    }

    if (pOneDev->typeList.count == pOneDev->typeList.capacity)
    {
        oneType_s *p = (oneType_s*) calloc((pOneDev->typeList.capacity + REAL_DATA_LIST_DELTA_SIZE), sizeof(oneType_s));
        if (p == NULL)
        {
            return -1;
        }

        for (i = 0; i < (pOneDev->typeList.capacity + REAL_DATA_LIST_DELTA_SIZE); i++)
        {
            initOneType(p + i, REAL_DATA_LIST_INIT_SIZE, -1, -1, -1);
        }

        memcpy(p, pOneDev->typeList.list, pOneDev->typeList.count * sizeof(oneType_s));
        free(pOneDev->typeList.list);
        pOneDev->typeList.list = p;
        pOneDev->typeList.capacity += REAL_DATA_LIST_DELTA_SIZE;
    }

    u32 idx = pOneDev->typeList.count;
    if (pOneDev->typeList.list[idx].append(&pOneDev->typeList.list[idx], pItem) == 0)
    {
        pOneDev->typeList.list[idx].seq = idx;
        pOneDev->typeList.list[idx].devNo = pItem->devNo;
        pOneDev->typeList.list[idx].type = pItem->type;
        pOneDev->typeList.count++;
        return 0;
    }

    return -1;
}

static int getItemByRealNoInOneDev(oneDevice_s *pOneDev, int realNo, dataItem_s *pDataItem)
{
    assert(pOneDev != NULL && pOneDev->typeList.list != NULL && pOneDev->typeList.capacity > 0 && pDataItem != NULL);

    int i = 0;
    for (i = 0; i < pOneDev->typeList.count; i++)
    {
        if (pOneDev->typeList.list[i].getItemByRealNo(&pOneDev->typeList.list[i], realNo, pDataItem) == 0)
        {
            return 0;
        }
    }

    return -1;
}

static void printOneDev(oneDevice_s *pOneDev, u32 depth)
{
    int i = 0;
    char depthstr[REAL_DATA_MAX_DEPTH + 1] = { 0 };
    for (i = 0; i < depth; i++)
    {
        strncat(depthstr, "\t", sizeof(depthstr) - 1);
    }

    printf("%s----------OneDev------------\n", depthstr);
    if (pOneDev == NULL)
    {
        printf("%spOneDev is NULL\n", depthstr);
        return;
    }

    printf("%sdevNo: %d\n", depthstr, pOneDev->devNo);
    printf("%scount: %d\n", depthstr, pOneDev->typeList.count);
    printf("%scapacity: %d\n", depthstr, pOneDev->typeList.capacity);

    for (i = 0; i < pOneDev->typeList.count; i++)
    {
        pOneDev->typeList.list[i].print(&pOneDev->typeList.list[i], depth + 1);
    }
}

static int initOneDev(oneDevice_s *pOneType, u32 size, int devNo, int linkNo, int type)
{
    assert(pOneType != NULL && size > 0);

    INIT_LIST(pOneType->typeList, oneType_s, size, NULL);

    int i = 0;
    for (i = 0; i < pOneType->typeList.capacity; i++)
    {
        initOneType(&pOneType->typeList.list[i], size, devNo, linkNo, type);
    }

    pOneType->free = freeOneDev;
    pOneType->append = addDataItemToOneDev;
    pOneType->print = printOneDev;
    pOneType->getItemByRealNo = getItemByRealNoInOneDev;

    return 0;
}

int realDBParse(const char *fullfilename, oneDevice_s *pOneDevice)
{
    assert(fullfilename != NULL && pOneDevice != NULL);

    struct LtuDevHead head;
    struct RealdatabasePara databasePara;

    int headLen = sizeof(head);
    int itemLen = sizeof(databasePara);
    int len = 0;
    int itemCount = 0;

    FILE *fd = fopen(fullfilename, "r");
    if (fd == NULL)
    {
        return -1;
    }

    struct stat st;
    if (stat(fullfilename, &st) != 0)
    {
        fclose(fd);
        return -1;
    }

    if (((st.st_size - headLen) % itemLen) != 0)
    {
        fclose(fd);
        return -1;
    }

    len = fread(&head, 1, headLen, fd);
    if (len != headLen)
    {
        fclose(fd);
        return -1;
    }

    itemCount = ((st.st_size - headLen) / itemLen);
    dataItem_s item = { 0 };

    if (initOneDev(pOneDevice, REAL_DATA_LIST_INIT_SIZE, -1, -1, -1) != 0)
    {
        return -1;
    }

    int i = 0;
    for (i = 0; i < itemCount; i++)
    {
        len = fread(&databasePara, 1, itemLen, fd);
        if (len != itemLen)
        {
            fclose(fd);
            return -1;
        }

        item.devNo = databasePara.devNo;
        item.type = databasePara.type;
        item.linkNo = databasePara.linkNo;
        item.realNo = databasePara.RealDevOrderNum;

        if (pOneDevice->append(pOneDevice, &item) != 0)
        {
            fclose(fd);
            pOneDevice->free(pOneDevice);
            return -1;
        }
    }

    fclose(fd);

    return 0;
}

void testInitDataList(void)
{
    oneDevice_s oneDev;
    realDBParse("/home/floyd/repo/mytesting/Debug/ltudevlib.dat", &oneDev);
    oneDev.print(&oneDev, 0);

    dataItem_s item = { 0 };
    oneDev.getItemByRealNo(&oneDev, 3, &item);
    printDataItem(&item, 0);

    oneDev.getItemByRealNo(&oneDev, 63, &item);
    printDataItem(&item, 0);

    oneDev.getItemByRealNo(&oneDev, 53, &item);
    printDataItem(&item, 0);
}
