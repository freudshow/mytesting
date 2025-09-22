#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int randmain()
{
    int fd;
    unsigned char random_bytes[16];

    // 打开 /dev/urandom 设备
    fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        perror("无法打开 /dev/urandom");
        return 1;
    }

    // 读取 16 字节的随机数据
    if (read(fd, random_bytes, 16) != 16) {
        perror("无法读取随机数据");
        close(fd);
        return 1;
    }

    // 关闭文件描述符
    close(fd);

    // 输出随机字节的十六进制表示
    for (int i = 0; i < 16; i++) {
        printf("%02X", random_bytes[i]);
        if (i != 15) {
            printf(":");
        }
    }
    printf("\n");

    return 0;
}
