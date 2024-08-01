#include "basedef.h"
#include <stdio.h>
#include <stdlib.h>

void printArray(int *array, int count)
{
	int i = 0;
	for(i = 0; i < count; i++)
	{
		printf("array[%d] = %d, ", i, array[i]);
	}

	printf("\n");
}

void bubbleSort(int *array, int count) {
	int temp = 0;

	int i = 0;
	int j = 0;

	for (i = 0; i < count; i++) {
		for (j = i + 1; j < count; j++) {
			if (array[i] > array[j]) {
				temp = array[i];
				array[i] = array[j];
				array[j] = temp;
				printArray(array, count);
			}
		}
	}
}

void testSort(void)
{
	int array[] = {5,12,11,4,9,45};
	int count = sizeof(array)/sizeof(array[0]);

	printArray(array, count);
	printf("-----------------------\n");
	bubbleSort(array, count);
	printf("-----------------------\n");
	printArray(array, count);
}
