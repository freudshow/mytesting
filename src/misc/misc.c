#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "basedef.h"

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

void sscanftest(void)
{
    char *pattern = "%04d.%[dat]";
    char *patternJson = "c%02d%02d.%[json]";

    u32 fileno = 0;
    char surfix[256] = { 0 };
    int count = sscanf("134.dat", pattern, &fileno, surfix);
    printf("count: %d, fileno: %d, surfix: %s\n", count, fileno, surfix);

    int linkno = 0;
    int prono = 0;

    count = sscanf("c0305.json", patternJson, &linkno, &prono, surfix);
    printf("count: %d, linkno: %d, prono: %d, surfix: %s\n", count, linkno, prono, surfix);
}

void testmisc(void)
{
    int res = createDir("/home/floyd/temp/mkdir/test/subdir/subdir1/subdir2/subdir3");

    printf("res: %d\n", res);
}
