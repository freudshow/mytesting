#include "basedef.h"
#include <string.h>

void testbase64(void)
{
    u8 buf[] = {0x05, 0x4e, 0x89, 0x5e, 0x8f, 0x11};

    char enStr[1024] = {0};

    int ret = encode_base64(buf, sizeof(buf), enStr);
    DEBUG_BUFF_FORMAT(buf, sizeof(buf), "enStr: %s", enStr);

    u8 debuf[1024] = {0};
    ret = decode_base64(enStr, strlen(enStr), debuf);
    DEBUG_BUFF_FORMAT(debuf, ret, "enStr: %s", enStr);
}
