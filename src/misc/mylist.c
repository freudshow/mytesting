#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

#define ADD_NODE_TO_LIST(list, node) \
    do { \
        (node)->next = NULL; \
        if ((list).head == NULL) { \
            (list).head = (node); \
            (list).tail = (node); \
        } else { \
            (list).tail->next = (node); \
            (list).tail = (node); \
        } \
        (list).size++; \
    } while (0)

#define REMOVE_NODE_FROM_LIST(list, node, prev) \
    do { \
        if (prev == NULL) { \
            (list).head = (node)->next; \
            if ((list).head == NULL) { \
                (list).tail = NULL; \
            } \
        } else { \
            prev->next = (node)->next; \
            if ((node)->next == NULL) { \
                (list).tail = prev; \
            } \
        } \
        free(node); \
        (list).size--; \
    } while (0)

typedef struct ListNode {
    int val;
    int seq;
    struct ListNode *next;
} ListNode_s;

typedef struct List {
    ListNode_s *head;
    ListNode_s *tail;
    int size;
} List_s;

void printList(List_s *list);
void freeList(List_s *list);

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

void insertSortList(List_s *list)
{
    if (list == NULL || list->head == NULL || list->head->next == NULL)
    {
        return; // 空链表或只有一个节点无需排序
    }

    ListNode_s *sortedHead = NULL; // 已排序部分的头指针
    ListNode_s *current = list->head; // 未排序部分的当前节点
    ListNode_s *nextNode = NULL; // 用于保存下一个待处理节点
    ListNode_s *prev = NULL; // 用于查找插入位置的指针

    List_s *testList = getEmptyList();
    while (current != NULL)
    {
        nextNode = current->next; // 保存下一个待处理节点（避免交换后丢失后续节点）
        printf("\n**********************************************\n");
        printf("current: val-%d, seq-%d\n----------------------------\n", current->val, current->seq);

        printf("sortedHead: \n\t");
        testList->head = sortedHead;
        printList(testList);
        printf("----------------------------\n");
        if (sortedHead == NULL ||
                current->val < sortedHead->val ||
                (current->val == sortedHead->val && current->seq < sortedHead->seq))
        { // 情况1：当前节点需要插入到已排序部分的头部
            current->next = sortedHead; // 当前节点指向已排序部分的头
            sortedHead = current; // 更新已排序部分的头为当前节点
        }
        else
        { // 情况2：当前节点需要插入到已排序部分的中间或尾部
            prev = sortedHead; // 用于查找插入位置的指针

            // 找到插入位置：prev的下一个节点比current大
            while (prev->next != NULL &&
                    (prev->next->val < current->val ||
                            (prev->next->val == current->val && prev->next->seq < current->seq)))
            {
                prev = prev->next;
            }

            // 插入current到prev和prev->next之间
            current->next = prev->next;
            prev->next = current;
        }

        printf("after insert into sortedHead: \n\t");
        testList->head = sortedHead;
        printList(testList);
        printf("**********************************************\n");
        current = nextNode; // 处理下一个未排序节点
    }

    free(testList);

    // 更新链表的头指针
    list->head = sortedHead;

    // 更新链表的尾指针（排序后遍历到最后一个节点）
    ListNode_s *tail = list->head;
    while (tail->next != NULL)
    {
        tail = tail->next;
    }
    list->tail = tail;
}

// 创建新节点
ListNode_s* createNode(int val, int seq)
{
    ListNode_s *node = (ListNode_s*) malloc(sizeof(ListNode_s));
    if (node == NULL)
    {
        printf("内存分配失败\n");
        exit(1);
    }
    node->val = val;
    node->seq = seq;
    node->next = NULL;
    return node;
}

// 合并两个有序链表
ListNode_s* merge(ListNode_s *left, ListNode_s *right)
{
    // 创建哨兵节点简化操作
    ListNode_s *dummy = createNode(0, 0);
    ListNode_s *current = dummy;

    while (left != NULL && right != NULL)
    {
        // 比较val，val相等则比较seq
        if (left->val < right->val ||
                (left->val == right->val && left->seq < right->seq))
        {
            current->next = left;
            left = left->next;
        }
        else
        {
            current->next = right;
            right = right->next;
        }
        current = current->next;
    }

    // 处理剩余节点
    if (left != NULL)
    {
        current->next = left;
    }
    else
    {
        current->next = right;
    }

    ListNode_s *result = dummy->next;
    free(dummy); // 释放哨兵节点
    return result;
}

// 找到链表的中间节点
ListNode_s* findMiddle(ListNode_s *head)
{
    if (head == NULL || head->next == NULL)
    {
        return head;
    }

    ListNode_s *slow = head;
    ListNode_s *fast = head->next;

    while (fast != NULL && fast->next != NULL)
    {
        slow = slow->next;
        fast = fast->next->next;
    }

    return slow;
}

// 归并排序递归函数
ListNode_s* mergeSort(ListNode_s *head)
{
    // 基本情况：空链表或只有一个节点
    if (head == NULL || head->next == NULL)
    {
        return head;
    }

    // 找到中间节点
    ListNode_s *middle = findMiddle(head);
    ListNode_s *nextOfMiddle = middle->next;

    // 拆分链表
    middle->next = NULL;

    // 递归排序两个子链表
    ListNode_s *left = mergeSort(head);
    ListNode_s *right = mergeSort(nextOfMiddle);

    // 合并排序后的子链表
    return merge(left, right);
}

// 对链表进行排序
void sortList(List_s *list)
{
    if (list == NULL || list->size <= 1)
    {
        return;
    }

    list->head = mergeSort(list->head);

    // 更新tail指针
    ListNode_s *current = list->head;
    while (current->next != NULL)
    {
        current = current->next;
    }
    list->tail = current;
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
        printf("[val-%d, seq-%d] -> ", current->val, current->seq);
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

    srand(time(NULL));
    // 添加节点
    for (int i = 1; i <= 20; i++)
    {
        ListNode_s *newNode = (ListNode_s*) malloc(sizeof(ListNode_s));
        if (newNode == NULL)
        {
            printf("Failed to allocate node\n");
            freeList(myList);
            return;
        }

        newNode->val = rand() % 100; // 随机值
        newNode->seq = i; // 顺序值
        addNodeToList(myList, newNode);
    }

    printList(myList);

//    // 查找节点
//    int searchVal = 30;
//    ListNode_s *foundNode = getNodeByValue(myList, searchVal);
//    if (foundNode != NULL)
//    {
//        printf("Found node with value %d\n", foundNode->val);
//    }
//    else
//    {
//        printf("Node with value %d not found\n", searchVal);
//    }
//
//    // 删除节点
//    int deleteVal = 20;
//    ListNode_s *deletedNode = deleteNodeByValue(myList, deleteVal);
//    if (deletedNode != NULL)
//    {
//        printf("Deleted node with value %d\n", deletedNode->val);
//        free(deletedNode); // 释放被删除节点的内存
//    }
//    else
//    {
//        printf("Node with value %d not found for deletion\n", deleteVal);
//    }
//
//    printf("after Delete node:\n");
//    printList(myList);
//
//    deleteVal = 10;
//    deletedNode = deleteNodeByValue(myList, deleteVal);
//    if (deletedNode != NULL)
//    {
//        printf("Deleted node with value %d\n", deletedNode->val);
//        free(deletedNode); // 释放被删除节点的内存
//    }
//    else
//    {
//        printf("Node with value %d not found for deletion\n", deleteVal);
//    }
//
//    printf("after Delete node:\n");
//    printList(myList);
//
//    deleteVal = 50;
//    deletedNode = deleteNodeByValue(myList, deleteVal);
//    if (deletedNode != NULL)
//    {
//        printf("Deleted node with value %d\n", deletedNode->val);
//        free(deletedNode); // 释放被删除节点的内存
//    }
//    else
//    {
//        printf("Node with value %d not found for deletion\n", deleteVal);
//    }
//
//    printf("after Delete node:\n");
//    printList(myList);
//
//    ListNode_s *insertNode = (ListNode_s*) malloc(sizeof(ListNode_s));
//    if (insertNode == NULL)
//    {
//        printf("Failed to allocate node\n");
//        freeList(myList);
//        return;
//    }
//    insertNode->val = 25;
//    ADD_NODE_TO_LIST(*myList, insertNode);
//    printf("after Insert node %d:\n", insertNode->val);
//    printList(myList);

    insertSortList(myList);
    printf("after Sort list:\n");
    printList(myList);

    // 清理列表
    freeList(myList);
}
