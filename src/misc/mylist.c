#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GENERIC_LIST(T) \
    typedef struct ListNode_##T { \
        T val; \
        struct ListNode_##T *next; \
    } ListNode_##T; \
    \
    typedef struct List_##T { \
        ListNode_##T *head; \
        ListNode_##T *tail; \
        int size; \
    } List_##T;

typedef struct ListNode {
    int val;
    struct ListNode *next;
} ListNode_s;

typedef struct List {
    ListNode_s *head;
    ListNode_s *tail;
    int size;
} List_s;

List_s* getEmptyList(void)
{
    List_s *list = (List_s*) malloc(sizeof(List_s));
    if (list == NULL)
    {
        return NULL; // 内存分配失败
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;

    return list;
}

void addNodeToList(List_s *list, ListNode_s *node)
{
    if (list == NULL || node == NULL)
    {
        return; // 无效参数
    }

    node->next = NULL; // 新节点的下一个节点设为NULL

    if (list->head == NULL)
    {
        // 列表为空，设置头和尾为新节点
        list->head = node;
        list->tail = node;
    }
    else
    {
        // 列表不为空，将新节点添加到尾部
        list->tail->next = node;
        list->tail = node;
    }

    list->size++;
}

ListNode_s* getNodeByValue(List_s *list, int val)
{
    if (list == NULL)
    {
        return NULL; // 无效参数
    }

    ListNode_s *current = list->head;
    while (current != NULL)
    {
        if (current->val == val)
        {
            return current; // 找到匹配的节点
        }

        current = current->next;
    }

    return NULL; // 未找到匹配的节点
}

ListNode_s* deleteNodeByValue(List_s *list, int val)
{
    if (list == NULL || list->head == NULL)
    {
        return NULL; // 无效参数或列表为空
    }

    ListNode_s *current = list->head;
    ListNode_s *previous = NULL;

    while (current != NULL)
    {
        if (current->val == val)
        {
            // 找到匹配的节点，进行删除
            if (previous == NULL)
            {
                // 删除头节点
                list->head = current->next;
                if (list->head == NULL)
                {
                    // 列表变为空，更新尾节点
                    list->tail = NULL;
                }
            }
            else
            {
                // 删除非头节点
                previous->next = current->next;
                if (current->next == NULL)
                {
                    // 删除的是尾节点，更新尾节点
                    list->tail = previous;
                }
            }

            list->size--;
            return current; // 返回被删除的节点
        }

        previous = current;
        current = current->next;
    }

    return NULL; // 未找到匹配的节点
}

void printList(List_s *list)
{
    if (list == NULL)
    {
        printf("List is NULL\n");
        return;
    }

    ListNode_s *current = list->head;
    printf("List (size=%d): ", list->size);
    while (current != NULL)
    {
        printf("%d -> ", current->val);
        current = current->next;
    }
    printf("NULL\n");
}

void freeList(List_s *list)
{
    if (list == NULL)
    {
        return; // 无效参数
    }

    ListNode_s *current = list->head;
    while (current != NULL)
    {
        ListNode_s *temp = current;
        current = current->next;
        free(temp); // 释放节点内存
    }

    free(list); // 释放列表结构体内存
}

void testMylist(void)
{
    List_s *myList = getEmptyList();
    if (myList == NULL)
    {
        printf("Failed to create list\n");
        return;
    }

    // 添加节点
    for (int i = 1; i <= 5; i++)
    {
        ListNode_s *newNode = (ListNode_s*) malloc(sizeof(ListNode_s));
        if (newNode == NULL)
        {
            printf("Failed to allocate node\n");
            freeList(myList);
            return;
        }
        newNode->val = i * 10;
        addNodeToList(myList, newNode);
    }

    printList(myList);

    // 查找节点
    int searchVal = 30;
    ListNode_s *foundNode = getNodeByValue(myList, searchVal);
    if (foundNode != NULL)
    {
        printf("Found node with value %d\n", foundNode->val);
    }
    else
    {
        printf("Node with value %d not found\n", searchVal);
    }

    // 删除节点
    int deleteVal = 20;
    ListNode_s *deletedNode = deleteNodeByValue(myList, deleteVal);
    if (deletedNode != NULL)
    {
        printf("Deleted node with value %d\n", deletedNode->val);
        free(deletedNode); // 释放被删除节点的内存
    }
    else
    {
        printf("Node with value %d not found for deletion\n", deleteVal);
    }

    printList(myList);

    // 清理列表
    freeList(myList);
}
