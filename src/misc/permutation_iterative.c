#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 栈元素结构体：存储当前路径和使用标记
typedef struct {
    int *path;          // 当前排列路径
    int *used;          // 记录元素是否被使用
    int pathLength;     // 当前路径长度
} StackElement;

// 生成全排列的迭代实现
int** permuteIterative(int *nums, int numsSize, int *returnSize, int **returnColumnSizes)
{
    // 计算全排列的总数：n!
    *returnSize = 1;
    for (int i = 1; i <= numsSize; i++)
    {
        *returnSize *= i;
    }

    // 分配结果数组内存
    int **result = (int**) malloc(*returnSize * sizeof(int*));

    // 记录每个排列的长度
    *returnColumnSizes = (int*) malloc(*returnSize * sizeof(int));
    for (int i = 0; i < *returnSize; i++)
    {
        (*returnColumnSizes)[i] = numsSize;
    }

    // 初始化栈
    StackElement *stack = (StackElement*) malloc(*returnSize * sizeof(StackElement));
    int stackTop = -1;  // 栈顶指针

    // 初始状态：空路径，所有元素均未使用
    StackElement initial;
    initial.path = (int*) malloc(numsSize * sizeof(int));
    initial.used = (int*) calloc(numsSize, sizeof(int)); // 初始化为0（未使用）
    initial.pathLength = 0;
    stack[++stackTop] = initial;

    *returnSize = 0; // 重置为0，用于计数

    // 处理栈中的元素
    while (stackTop >= 0)
    {
        // 弹出栈顶元素
        StackElement current = stack[stackTop--];

        // 如果路径长度等于数组长度，说明找到一个完整排列
        if (current.pathLength == numsSize)
        {
            result[*returnSize] = (int*) malloc(numsSize * sizeof(int));
            memcpy(result[*returnSize], current.path, numsSize * sizeof(int));
            (*returnSize)++;

            // 释放当前元素的内存
            free(current.path);
            free(current.used);
            continue;
        }

        // 遍历所有元素，尝试加入路径
        for (int i = 0; i < numsSize; i++)
        {
            if (!current.used[i])
            { // 如果元素未被使用
              // 创建新的路径和使用标记
                StackElement newElement;
                newElement.path = (int*) malloc(numsSize * sizeof(int));
                newElement.used = (int*) malloc(numsSize * sizeof(int));
                newElement.pathLength = current.pathLength + 1;

                // 复制当前路径和使用标记
                memcpy(newElement.path, current.path, current.pathLength * sizeof(int));
                memcpy(newElement.used, current.used, numsSize * sizeof(int));

                // 选择当前元素
                newElement.path[current.pathLength] = nums[i];
                newElement.used[i] = 1; // 标记为已使用

                // 将新状态入栈
                stack[++stackTop] = newElement;
            }
        }

        // 释放当前元素的内存
        free(current.path);
        free(current.used);
    }

    free(stack); // 释放栈内存
    return result;
}

// 测试函数
void testPermIter(void)
{
    int nums[] = { 1, 2, 3 };
    int numsSize = sizeof(nums) / sizeof(nums[0]);
    int returnSize;
    int *returnColumnSizes;

    int **result = permuteIterative(nums, numsSize, &returnSize, &returnColumnSizes);

    printf("迭代版本 - 数字 [1,2,3] 的全排列为：\n");
    for (int i = 0; i < returnSize; i++)
    {
        printf("%d. [", i + 1);
        for (int j = 0; j < returnColumnSizes[i]; j++)
        {
            printf("%d", result[i][j]);
            if (j < returnColumnSizes[i] - 1)
            {
                printf(", ");
            }
        }
        printf("]\n");
        free(result[i]); // 释放每个排列的内存
    }

    free(result); // 释放结果数组内存
    free(returnColumnSizes);
}
