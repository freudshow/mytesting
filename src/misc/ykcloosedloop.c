#include "basedef.h"
#include <sys/time.h>

typedef struct {
    int isValid;//配置是否有效, 1:有效，0:无效
    int linkNo;//link号
    int devNo;//设备号
    int areaNo;//区域号
    int delayAfterRcvMsg;//接收到遥控指令后的延时, 单位毫秒ms。有的遥控/遥调下发后，
                         //逆变器/设备不是立即有反应，需要等待一段时间再采集遥信/遥测等数值。
    int sendCount;//发送几次
    int reserve[20];//保留
}ykClosedLoopYxSet_s;

typedef struct YKClosedLoopYxQueenItem *closedYkQueenItem_p;
/**
 * 遥控闭环采集遥信的链表元素
 */
typedef struct YKClosedLoopYxQueenItem {
    u64 startTime;                      // 接收到遥控消息的开始时间
    ykClosedLoopYxSet_s config;         // 遥控闭环的配置
    closedYkQueenItem_p next;           // 元素的后继
}closedYkQueenItem_s;

static void printList(closedYkQueenItem_p head)
{
    if (head == NULL)
    {
        printf("NULL List\n");
        return;
    }

    printf("head: %p\r\n", head);

    closedYkQueenItem_p pItem = head->next;
    while (pItem != NULL)
    {
        printf("item: %p\n", pItem);
        printf("\tstartTime: %lld\n", pItem->startTime);
        printf("\t\tlinkNo: %d\n", pItem->config.linkNo);
        printf("\t\tdevNo: %d\n", pItem->config.devNo);
        printf("\t\tareaNo: %d\n", pItem->config.areaNo);
        printf("\t\tdelayAfterRcvMsg: %d\n", pItem->config.delayAfterRcvMsg);
        printf("\t\tsendCount: %d\n", pItem->config.sendCount);

        pItem = pItem->next;
    }
}

void testLinkedList(void)
{
    ykClosedLoopYxSet_s configArray[] = {
            {1, 8, 1,  5, 2000, 3},
            {1, 8, 2,  5, 5000, 3},
            {1, 8, 3,  5, 23000, 3},
            {1, 8, 4,  5, 6000, 3},
            {1, 8, 5,  5, 8000, 3},
            {1, 8, 6,  5, 12000, 3},
            {1, 8, 7,  5, 22000, 3},
            {1, 8, 8,  5, 11000, 3},
            {1, 8, 9,  5, 19000, 3},
            {1, 8, 10, 5, 3000, 3},
            {1, 8, 11, 5, 2000, 3},
            {1, 8, 12, 5, 6000, 3},
            {1, 8, 13, 5, 14000, 3},
            {1, 8, 14, 5, 10000, 3},
            {1, 8, 15, 5, 7000, 3},
    };

    const u32 count = sizeof(configArray) / sizeof(configArray[0]);

    closedYkQueenItem_s ListHead = { 0 };
    closedYkQueenItem_p pItem = NULL;
    struct timeval tv;

    for (int i = 0; i < count; i++)
    {
        pItem = (closedYkQueenItem_p) calloc(1, sizeof(closedYkQueenItem_s));
        memcpy(&pItem->config, &configArray[i], sizeof(ykClosedLoopYxSet_s));

        gettimeofday(&tv, NULL);

        pItem->startTime = tv.tv_sec * 1000 + tv.tv_usec / 1000; //记录开始时间

        //pItem放到链表头部
        pItem->next = ListHead.next; //pItem的后继指向第一个元素
        ListHead.next = pItem; //将链表头的后继指向pItem
    }

    printList(&ListHead);

    pItem = NULL; //获取第一个元素
    closedYkQueenItem_p prevItem = NULL; //获取当前元素的前驱
    u64 diff = 0;
    int listCount = count;

    while(listCount > 0)
    {
        pItem = ListHead.next; //获取第一个元素
        prevItem = &ListHead; //获取当前元素的前驱
        while (pItem != NULL)
        {
            gettimeofday(&tv, NULL);
            diff = tv.tv_sec * 1000 + tv.tv_usec / 1000 - pItem->startTime;
            if (diff >= pItem->config.delayAfterRcvMsg)
            {
                DEBUG_TIME_LINE("item: %p, devNo: %d, to be delete, before delete list count: %d",
                        pItem, pItem->config.devNo, listCount);
                //删除pItem
                if (prevItem != NULL)
                {
                    prevItem->next = pItem->next; //prevItem的后继指向pItem的后继
                }

                free(pItem);

                pItem = prevItem->next; //pItem指向prevItem的后继
                listCount--;
                printList(&ListHead);
            }
            else
            {
                prevItem = pItem;
                pItem = pItem->next;
            }
        }
    }

    printf("after delete, listCount: %d, ListHead: %p, ListHead.next: %p\n", listCount, &ListHead, ListHead.next);

    printf("now, rebuild list\n");
    for (int i = 0; i < count; i++)
    {
        pItem = (closedYkQueenItem_p) calloc(1, sizeof(closedYkQueenItem_s));
        memcpy(&pItem->config, &configArray[i], sizeof(ykClosedLoopYxSet_s));

        gettimeofday(&tv, NULL);

        pItem->startTime = tv.tv_sec * 1000 + tv.tv_usec / 1000; //记录开始时间

        //pItem放到链表头部
        pItem->next = ListHead.next; //pItem的后继指向第一个元素
        ListHead.next = pItem; //将链表头的后继指向pItem
    }

    printList(&ListHead);
}
