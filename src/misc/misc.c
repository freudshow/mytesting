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

int getBaseFileName(const char *appname, int len, int *namestart)
{
    int pos = len - 1;

    while (appname[pos] == '/' && pos >= 0)
    {
        pos--;
    }

    if (pos < 0)
    {
        DEBUG_TIME_LINE("pos: %d\n", pos);
        return -1;
    }

    if (appname[pos] == '.')
    {
        if (pos == 1 && appname[0] == '.')
        {
            *namestart = 0;
            return 2;
        }
        else if (pos == 0)
        {
            *namestart = 0;
            return 1;
        }

        return -1;
    }

    int endPos = pos;
    DEBUG_TIME_LINE("endPos: %d, char: %c\n", endPos, appname[endPos]);
    while (appname[pos] != '/' && pos >= 0)
    {
        DEBUG_TIME_LINE("start: %d, char: %c\n", pos, appname[pos]);
        pos--;
    }

    int namelen = endPos - pos;
    *namestart = pos + 1;
    DEBUG_TIME_LINE("namestart: %d, namelen: %d\n", *namestart, namelen);

    return namelen;
}

void testmisc(void)
{
    char *path = "////";

    int start = 0;
    int len = getBaseFileName(path, strlen(path), &start);

    DEBUG_TIME_LINE("start: %d, name len: %d\n", start, len);

    if (len >= 0)
    {
        char name[1024] = { 0 };
        memcpy(name, &path[start], len);
        DEBUG_TIME_LINE("name: %s\n", name);
    }
}
