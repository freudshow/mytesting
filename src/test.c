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

int main(int argc, char **argv)
{
//    testjansson();
//    testbase64();
//    testInitDataList();
    testtcp();
//    OLED_test();
//	  testmydb();
//    testjson();

    return 0;
}
