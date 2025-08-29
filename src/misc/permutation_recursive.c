#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 交换两个整数
void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

// 递归生成全排列
void backtrack(int *nums, int start, int length, int **result, int *returnSize)
{
    // 终止条件：当start等于数组长度时，找到一个完整排列
    if (start == length)
    {
        // 为当前排列分配内存并复制
        result[*returnSize] = (int*) malloc(length * sizeof(int));
        memcpy(result[*returnSize], nums, length * sizeof(int));
        (*returnSize)++;
        return;
    }

    // 遍历所有可能的元素
    for (int i = start; i < length; i++)
    {
        // 选择当前元素
        swap(&nums[start], &nums[i]);

        // 递归处理下一个位置
        backtrack(nums, start + 1, length, result, returnSize);

        // 回溯：撤销选择
        swap(&nums[start], &nums[i]);
    }
}

// 生成全排列的入口函数
int** permuteRecursive(int *nums, int numsSize, int *returnSize, int **returnColumnSizes)
{
    // 计算全排列的总数：n!
    *returnSize = 1;
    for (int i = 1; i <= numsSize; i++)
    {
        *returnSize *= i;
    }

    // 分配结果数组内存
    int **result = (int**) malloc(*returnSize * sizeof(int*));

    // 记录每个排列的长度（都是numsSize）
    *returnColumnSizes = (int*) malloc(*returnSize * sizeof(int));
    for (int i = 0; i < *returnSize; i++)
    {
        (*returnColumnSizes)[i] = numsSize;
    }

    // 调用回溯函数生成全排列
    *returnSize = 0; // 重置为0，用于计数
    backtrack(nums, 0, numsSize, result, returnSize);

    return result;
}

// 测试函数
void testPermRecur(void)
{
    int nums[] = { 1, 2, 3 };
    int numsSize = sizeof(nums) / sizeof(nums[0]);
    int returnSize;
    int *returnColumnSizes;

    int **result = permuteRecursive(nums, numsSize, &returnSize, &returnColumnSizes);

    printf("递归版本 - 数字 [1,2,3] 的全排列为：\n");
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
