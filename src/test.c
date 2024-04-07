#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "basedef.h"

//void testjansson(void);
//void testInitDataList(void);
//void testbase64(void);
//void OLED_test(void);
void testtcp(void);
//void testjson(void);
//u32 crc32File(const char *fullname);
int getWeekByDate(int year, int month, int day);

int main(int argc, char **argv)
{
//    testjansson();
//    testbase64();
//    testInitDataList();
    testtcp();
//    OLED_test();
//	  testmydb();
//    testjson();

    int week = getWeekByDate(2024, 4, 9);
    printf("weekday: %d\n", week);

    return 0;
}
