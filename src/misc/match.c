/*
 * match.c
 *
 *  Created on: Sep 25, 2024
 *      Author: floyd
 */
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>

int matchreg(const char *str, const char *pattern)
{
    regex_t re;
    int err;

    //使用扩展正则表达式
    err = regcomp(&re, pattern, REG_EXTENDED);
    if (err != 0)
    {
        char buf[100];
        regerror(err, &re, buf, sizeof(buf));
        printf("FAIL: %s\n", buf);
        return 0;
    }

    err = regexec(&re, str, 0, NULL, 0);
    regfree(&re);

    if (err)
    {
        return 1;
    }

    return 0;
}

void testMatch(void)
{
    char pattern[] = "^c[0-9]{4}.json$";
    char str[] = "c3494.json";

    int res = matchreg(str, pattern);

    printf("res: %d\n", res);
}
