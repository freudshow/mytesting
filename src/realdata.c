#include "basedef.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define REAL_DATA_LIST_INIT_SIZE    10  //列表初始大小
#define REAL_DATA_LIST_DELTA_SIZE    5  //列表增量大小

#define REAL_DATA_MAX_DEPTH           128//最大深度

typedef struct dataItemStruct {
    int devNo;         //设备号
    int linkNo;        //链路号
    int type;          //遥测遥信类型
    int realNo;        //实时库号
    float value;       //实时库值
    char modeName[16]; //模型名, 比如"YX_0", "YC_34"
} dataItem_s;

typedef A_LIST_OF(dataItem_s) dataItemList_s;
struct dataListStruct;
typedef struct dataListStruct dataList_s;

/**
 * 相同遥测遥信类型, 且实时库连续
 * 的各个数据项列表
 */
typedef struct dataListStruct {
    int devNo;                      //设备号
    int linkNo;                     //链路号
    int type;                       //遥测遥信类型
    dataItemList_s dataItemList;    //实时库列表
    int (*init)(dataList_s *pList, u32 size, int devNo, int linkNo, int type);
    int (*append)(dataList_s *pList, dataItem_s *pDataItem, u32 idx);
    int (*getItemByRealNo)(dataList_s *pList, int realNo, dataItem_s *pDataItem);
    int (*getFirst)(dataList_s *pList, dataItem_s *pDataItem);
    int (*getLast)(dataList_s *pList, dataItem_s *pDataItem);
    int (*remove)(dataList_s *pList, u32 idx);
    int (*modify)(dataItem_s *pDataItem, u32 idx);
    void (*print)(dataList_s *pList, u32 depth);
} dataList_s;

typedef A_LIST_OF(dataList_s) divDataItemList_s;

/**
 * 一个遥测遥信类型
 */
typedef struct oneTypeListStruct {
    int devNo;                      //设备号
    int linkNo;                     //链路号
    int type;                       //遥测遥信类型
    divDataItemList_s divDataList;  //不连续的实时库列表
} oneType_s;

typedef A_LIST_OF(oneType_s) typeList_s;

/**
 * 一个链路
 */
typedef struct oneLinkStruct {
    int devNo;              //设备号
    int linkNo;             //链路号
    typeList_s typeList;    //遥测遥信类型列表
} oneLink_s;

typedef A_LIST_OF(oneLink_s) linkList_s;

/**
 * 一个设备
 */
typedef struct oneDeviceStruct {
    int devNo;              //设备号
    linkList_s linkList;    //链路列表
} oneDevice_s;

typedef A_LIST_OF(oneDevice_s) devList_s;//设备列表, 总的实时库表


static devList_s devList;//设备列表, 总的实时库表

static int appendDataItem(dataList_s *pList, dataItem_s *pDataItem, u32 idx)
{
    if (pList == NULL || pDataItem == NULL)
    {
        return -1;
    }

    int i = 0;

    if (pList->dataItemList.list == NULL || pList->dataItemList.capacity == 0)
    {
        return -1;
    }

    if (idx > pList->dataItemList.count)
    {
        return -1;
    }

    if (pList->dataItemList.count == pList->dataItemList.capacity)
    {
        dataItem_s *p = (dataItem_s*) calloc((pList->dataItemList.capacity + 5), sizeof(dataItem_s));
        if (p == NULL)
        {
            return -1;
        }

        memcpy(p, pList->dataItemList.list, pList->dataItemList.count * sizeof(dataItem_s));
        pList->dataItemList.free(pList->dataItemList.list);
        pList->dataItemList.list = p;
        pList->dataItemList.capacity += 5;
    }

    if (idx == pList->dataItemList.count)
    {
        memcpy(&pList->dataItemList.list[idx], pDataItem, sizeof(dataItem_s));
        pList->dataItemList.count++;
        return 0;
    }

    for (i = pList->dataItemList.count; i > (idx + 1); i--)
    {
        memcpy(&pList->dataItemList.list[i], &pList->dataItemList.list[i - 1], sizeof(dataItem_s));
    }

    memcpy(&pList->dataItemList.list[idx], pDataItem, sizeof(dataItem_s));
    pList->dataItemList.count++;

    return 0;
}

static int getItemByRealNo(dataList_s *pList, int realNo, dataItem_s *pDataItem)
{
    if (pList == NULL || pDataItem == NULL || pList->dataItemList.list == NULL || pList->dataItemList.count == 0 || pList->dataItemList.capacity == 0)
    {
        return -1;
    }

    int i = 0;
    for (i = 0; i < pList->dataItemList.count; i++)
    {
        if (pList->dataItemList.list[i].realNo == realNo)
        {
            memcpy(pDataItem, &pList->dataItemList.list[i], sizeof(dataItem_s));
            return 0;
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
    if (pList == NULL || pDataItem == NULL || pList->dataItemList.list == NULL || pList->dataItemList.count == 0 || pList->dataItemList.capacity == 0)
    {
        return -1;
    }

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

    printf("%s--------------\n", depthstr);
    if (pItem == NULL)
    {
        printf("%spItem is NULL\n", depthstr);
        return;
    }

    printf("%sdevNo: %d\n", depthstr, pItem->devNo);
    printf("%slinkNo: %d\n", depthstr, pItem->linkNo);
    printf("%stype: %d\n", depthstr, pItem->type);
    printf("%srealNo: %d\n", depthstr, pItem->realNo);
    printf("%svalue: %f\n", depthstr, pItem->value);
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

    printf("%s--------------\n", depthstr);
    if (pList == NULL)
    {
        printf("%spList is NULL\n", depthstr);
        return;
    }

    printf("%sdevNo: %d, linkNo: %d, type: %d\n", depthstr, pList->devNo, pList->linkNo, pList->type);
    printf("%sList: %p, count: %d, capacity: %d\n", depthstr, pList->dataItemList.list, pList->dataItemList.count, pList->dataItemList.capacity);

    for (i = 0; i < pList->dataItemList.count; i++)
    {
        printDataItem(&pList->dataItemList.list[i], depth + 1);
    }
}

static int initDataList(dataList_s *pList, u32 size, int devNo, int linkNo, int type)
{
    if (pList == NULL || size == 0)
    {
        return -1;
    }

    INIT_LIST(pList->dataItemList, dataItem_s, size, free);

    if (pList->dataItemList.list == NULL)
    {
        return -1;
    }

    pList->devNo = devNo;
    pList->linkNo = linkNo;
    pList->type = type;

    pList->append = appendDataItem;
    pList->getItemByRealNo = getItemByRealNo;
    pList->getFirst = getFirstDataItem;
    pList->getLast = getLastDataItem;
    pList->print = printDataList;

    return 0;
}

static int initOneType(devList_s *pList)
{
    if (pList == NULL)
    {
        return -1;
    }

    return 0;
}

static int initOneLink(devList_s *pList)
{
    if (pList == NULL)
    {
        return -1;
    }

    return 0;
}

static int initOneDev(devList_s *pList)
{
    if (pList == NULL)
    {
        return -1;
    }

    return 0;
}

void testInitDataList(void)
{
    dataList_s dataList;
    initDataList(&dataList, REAL_DATA_LIST_INIT_SIZE, 0, 1, 10);

    dataItem_s list[] = {
            {0, 1, 0, 0, 0.0, "YX_0"},
            {0, 1, 0, 1, 0.0, "YX_1"},
            {0, 1, 0, 2, 0.0, "YX_2"},
            {0, 1, 0, 3, 1.0, "YX_3"},
            {0, 1, 0, 4, 1.0, "YX_4"},
            {0, 1, 0, 5, 0.0, "YX_5"},
            {0, 1, 0, 6, 1.0, "YX_6"},
            {0, 1, 0, 7, 0.0, "YX_7"},
            {0, 1, 0, 8, 0.0, "YX_8"},
            {0, 1, 0, 9, 1.0, "YX_9"},
            {0, 1, 0, 10, 0.0, "YX_10"},
            {0, 1, 0, 11, 1.0, "YX_11"},
            {0, 1, 0, 12, 1.0, "YX_12"},
            {0, 1, 0, 13, 0.0, "YX_13"},
            {0, 1, 0, 14, 0.0, "YX_14"},
            {0, 1, 0, 15, 1.0, "YX_15"},
            {0, 1, 0, 16, 0.0, "YX_16"},
            {0, 1, 0, 17, 0.0, "YX_17"},
            {0, 1, 0, 18, 0.0, "YX_18"},
            {0, 1, 0, 19, 0.0, "YX_19"},
            {0, 1, 0, 20, 0.0, "YX_20"},
            {0, 1, 0, 21, 0.0, "YX_21"},
            {0, 1, 0, 22, 0.0, "YX_22"},
            {0, 1, 0, 23, 0.0, "YX_23"},
            {0, 1, 1, 24, 2.0, "YC_0"},
            {0, 1, 1, 25, 3.0, "YC_1"},
            {0, 1, 1, 26, 4.0, "YC_2"},
            {0, 1, 1, 27, 5.0, "YC_3"},
            {0, 1, 1, 28, 6.0, "YC_4"},
            {0, 1, 1, 29, 7.0, "YC_5"},
            {0, 1, 1, 30, 8.0, "YC_6"},
            {0, 1, 1, 31, 9.0, "YC_7"},
            {0, 1, 1, 32, 10.0, "YC_8"},
            {0, 1, 1, 33, 11.0, "YC_9"},
            {0, 1, 1, 34, 12.0, "YC_10"},
            {0, 1, 1, 35, 13.0, "YC_11"},
            {0, 1, 1, 36, 14.0, "YC_12"},
            {0, 1, 1, 37, 15.0, "YC_13"},
            {0, 1, 1, 38, 16.0, "YC_14"},
            {0, 1, 1, 39, 17.0, "YC_15"},
            {0, 1, 1, 40, 18.0, "YC_16"},
            {0, 1, 1, 41, 19.0, "YC_17"},
            {0, 1, 1, 42, 20.0, "YC_18"},
            {0, 1, 1, 43, 21.0, "YC_19"},
            {0, 1, 1, 44, 22.0, "YC_20"},
            {0, 1, 1, 45, 23.0, "YC_21"},
            {0, 1, 1, 46, 24.0, "YC_22"},
            {0, 1, 1, 47, 25.0, "YC_23"},
            {0, 1, 1, 48, 26.0, "YC_24"},
            {0, 1, 1, 49, 27.0, "YC_25"},
            {0, 1, 1, 50, 28.0, "YC_26"},
            {0, 1, 1, 51, 29.0, "YC_27"},
            {0, 1, 1, 52, 30.0, "YC_28"},
            {0, 1, 1, 53, 31.0, "YC_29"},
            {0, 1, 1, 54, 32.0, "YC_30"},
            {0, 1, 1, 55, 33.0, "YC_31"},
            {0, 1, 1, 56, 34.0, "YC_32"},
            {0, 1, 1, 57, 35.0, "YC_33"},
            {0, 1, 1, 58, 36.0, "YC_34"},
            {0, 1, 1, 59, 37.0, "YC_35"},
            {0, 1, 1, 60, 38.0, "YC_36"},
            {0, 1, 1, 61, 39.0, "YC_37"},
            {0, 1, 1, 62, 40.0, "YC_38"},
            {0, 1, 1, 63, 41.0, "YC_39"},
            {0, 1, 1, 64, 42.0, "YC_40"},
            {0, 1, 1, 65, 43.0, "YC_41"},
            {0, 1, 1, 66, 44.0, "YC_42"},
            {0, 1, 1, 67, 45.0, "YC_43"},
            {0, 1, 1, 68, 46.0, "YC_44"},
            {0, 1, 1, 69, 47.0, "YC_45"},
    };

    int i = 0;
    for (i = 0; i < NELEM(list); i++)
    {
        dataList.append(&dataList, &list[i], i);
    }

    dataList.print(&dataList, 0);
    dataList.append(&dataList, &list[10], 10);
    dataList.print(&dataList, 0);

    dataItem_s item = { 0 };

    dataList.getItemByRealNo(&dataList, 10, &item);
    printDataItem(&item, 0);
    dataList.getItemByRealNo(&dataList, 17, &item);
    printDataItem(&item, 0);
    dataList.getItemByRealNo(&dataList, 0, &item);
    printDataItem(&item, 0);
    dataList.getItemByRealNo(&dataList, 70, &item);
    printDataItem(&item, 0);
    dataList.getItemByRealNo(&dataList, 71, &item);
    printDataItem(&item, 0);
    dataList.getItemByRealNo(&dataList, 72, &item);
    printDataItem(&item, 0);

    dataList.getFirst(&dataList, &item);
    printDataItem(&item, 0);
    dataList.getLast(&dataList, &item);
    printDataItem(&item, 0);
}
