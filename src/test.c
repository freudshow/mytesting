#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "basedef.h"

//void testjansson(void);
//void testInitDataList(void);
//void testbase64(void);
//void OLED_test(void);
//void testtcp(void);
//void testjson(void);
u32 crc32File(const char *fullname);
u8 buf[64*1024*1024];
int main(int argc, char **argv)
{
//    testjansson();
//    testbase64();
//    testInitDataList();
//    testtcp();
//    OLED_test();
//	  testmydb();
//    testjson();

    u32 crc = crc32File("/home/floyd/repo/busybox-1.36.1.tar.bz2");
    printf("crc32: %08X\n", crc);

    int fd = open("/home/floyd/repo/busybox-1.36.1.tar.bz2", O_RDONLY, 0777);

    u32 len = read(fd, buf, sizeof(buf));
    crc = crc32(buf, len);
    printf("crc32: %08X\n", crc);

    return 0;
}
