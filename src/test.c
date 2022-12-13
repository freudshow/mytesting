/*
 ============================================================================
 Name        : test.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "list.h"
#include "basedef.h"
#include "oled.h"
#include "tcp.h"

int main(int argc, char **argv)
{
    u32 delta = 5;
    int item = 0;
    A_LIST_OF(int) vlist;
    INIT_LIST(vlist, int, 10, free);

    int idx = 0;
    int tempit = 0;

    for (idx = 0; idx < vlist.capacity; idx++)
    {
        vlist.list[idx] = idx;
        vlist.count++;
    }

    idx = 9;
    item = 99999;
    INSERT_ITEM_TO_LIST(vlist, idx, item, delta, int, tempit);

    for (idx = 0; idx < vlist.count; idx++)
    {
        printf("list[%d]: %d\n", idx, vlist.list[idx]);
    }

    printf("count: %d\n", vlist.count);

    DELETE_LIST_ONE(vlist, 6, tempit);

    for (idx = 0; idx < vlist.count; idx++)
    {
        printf("list[%d]: %d\n", idx, vlist.list[idx]);
    }

    printf("count: %d\n", vlist.count);

    return 0;
}
