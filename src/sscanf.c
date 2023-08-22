#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

char* my_strstr(const char *s1, const char *s2)
{
    int n;

    if (*s2)
    {
        while (*s1)
        {
            for (n = 0; *(s1 + n) == *(s2 + n); n++)
            {
                if (!*(s2 + n + 1))
                    return (char*) s1;
            }

            s1++;
        }

        return NULL;
    }
    else
    {
        return (char*) s1;
    }
}

int main(int argc, char *argv[])
{
    char *src = "hi i am fucker you do!";
    char *dst = "fuck";
    char *ret = my_strstr(src, dst);
    printf("%s\n", ret);

    char TmpInBuf[512]="\r\n+CSQ: 13,99\r\nOK\r\n";
    char TmpOutBuf[256] = {0};
    int rssi = 0;
    int ber = 0;

    int count = sscanf((char *)TmpInBuf, "%63[^CSQ:]CSQ: %2d,%2d", (char *)TmpOutBuf, &rssi, &ber);

    printf("count: %d, TmpOutBuf: %s, rssi: %d, ber: %d\n", count, TmpOutBuf, rssi, ber);

//    char ccid[] = "\r\n+CSQ: 14,99\r\nOK\r\n +CCID: 89860122801210349713\r\nOK\r\n";
    char ccid[] = "\r\n+CCID: 89860122801210349713\r\nOK\r\n";
    char pOutCCID[128] = {0};
//    count = sscanf(ccid, "%255[^CCID]CCID: %63[0123456789abcdefABCDEF]", TmpOutBuf, pOutCCID);
//    printf("count: %d, TmpOutBuf: %s, ccid: %s\n", count, TmpOutBuf, pOutCCID);
    count = sscanf(ccid, "%*[^CCID:]CCID: %127[0123456789abcdefABCDEF]", pOutCCID);
    printf("\n[%s][%d]count: %d, pOutCCID: %s\n", __FUNCTION__, __LINE__, count, pOutCCID);

    return 0;
}
