#include "basedef.h"
#include <stdio.h>
#include <stdlib.h>

void printArray(int *array, int count)
{
    int i = 0;
    for (i = 0; i < count; i++)
    {
        printf("array[%d] = %d, ", i, array[i]);
    }

    printf("\n");
}

void bubbleSort(int *array, int count)
{
    int temp = 0;

    int i = 0;
    int j = 0;

    for (i = 0; i < count; i++)
    {
        for (j = i + 1; j < count; j++)
        {
            if (array[i] > array[j])
            {
                temp = array[i];
                array[i] = array[j];
                array[j] = temp;
                printArray(array, count);
            }
        }
    }
}

void insertSort(int *array, int count)
{
    int key = 0;

    int i = 0;
    int j = 0;

    for (i = 1; i < count; i++)
    {
        key = array[i];//array[i] may be replaced by an element in array[0...i-1]
        j = i - 1;
        while (j >= 0 && array[j] > key)
        {
            array[j + 1] = array[j];
            j--;
        }

        array[j + 1] = key;

        printArray(array, count);
    }
}

void testSort(void)
{
    int array[] = { 5, 12, 11, 4, 9, 45 };
    int count = sizeof(array) / sizeof(array[0]);

    printArray(array, count);
    printf("-----------------------\n");
    insertSort(array, count);
    printf("-----------------------\n");
    printArray(array, count);
}
