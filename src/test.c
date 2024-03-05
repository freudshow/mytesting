#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

#include "basedef.h"

//void testjansson(void);
//void testInitDataList(void);
//void testbase64(void);
//void OLED_test(void);
//void testtcp(void);
void testjson(void);

int main(int argc, char **argv)
{
//    testjansson();
//    testbase64();
//    testInitDataList();
//    testtcp();
//    OLED_test();
//	  testmydb();
//    testjson();

    int i = 0;
    short int j[3] = { 0 };
    short int *pint = j;
    unsigned char buff[] = { 0x00, 0x64, 0x00, 0x70, 0x00, 0x91 };
    for(i =0;i<3;i++)
    {
        *(pint + i) = ((*(char*) (buff + 2 * i) & 0xff) << 8) | (*(char*) (buff + 1 + 2 * i));
        printf("%d,%4x,%d\n",j[i],j[i],*(pint + i));
    }

    return 0;
}
