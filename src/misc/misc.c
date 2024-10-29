#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int createDir(const char *s)
{
    char dirName[256];
    strcpy(dirName, s);
    int i, len = strlen(dirName);
    if (dirName[len - 1] != '/')
        strcat(dirName, "/");

    len = strlen(dirName);

    for (i = 1; i < len; i++)
    {
        if (dirName[i] == '/')
        {
            dirName[i] = 0;
            if (access(dirName, F_OK) != 0)
            {
                if (mkdir(dirName, 0755) != 0)
                {
                    perror("mkdir   error");
                    return -1;
                }
            }

            dirName[i] = '/';
        }
    }

    return 0;
}

void testmisc(void)
{
    int res = createDir("/home/floyd/temp/mkdir/test/subdir/subdir1/subdir2/subdir3");

    printf("res: %d\n", res);
}
