#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h> // For basename (POSIX)
#include "basedef.h"

//设置某一位为1
#define SET_BIT(value, bit) ((value) | (1U << (bit)))

//设置某一位为0
#define CLEAR_BIT(value, bit) ((value) & ~(1U << (bit)))

char* get_filename(const char *path)
{
#ifdef _WIN32
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    _splitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
    size_t filename_len = strlen(fname) + strlen(ext) + 1; // +1 for null terminator
    char* filename = (char*)malloc(filename_len);
    if (filename == NULL) {
        perror("Memory allocation failed");
        return NULL; // Or handle the error as needed
    }
    strcpy_s(filename, filename_len, fname);
    strcat_s(filename, filename_len, ext);
    return filename;

#else // POSIX (Linux, macOS, etc.)
    char *filename = strdup(basename((char*) path)); //strdup to make a copy
    if (filename == NULL)
    {
        perror("Memory allocation failed");
        return NULL;
    }

    return filename;
#endif
}

void print_binary(u32 num)
{
    for (int i = sizeof(int) * 8 - 1; i >= 0; i--)
    {
        printf("%d", (num >> i) & 1);
    }

    printf("\n");
}

int filename(void)
{
//    const char *filepath = __FILE__;
//    char *filename = get_filename(filepath);
//
//    if (filename != NULL)
//    {
//        printf("Filename: %s\n", filename);
//        free(filename); // Free the allocated memory
//    }

    printf("fullname: %s\n", __FILE__);
    printf("basename: %s\n", basename(__FILE__));

    u32 a = 0;
    u32 b = 0xFFFFFFFF;
    for (int i = 0; i < 32; i++)
    {
        a = CLEAR_BIT(b, i);
        printf("%04X, \t", a);
        print_binary(a);
    }

    b = 0;
    for (int i = 0; i < 32; i++)
    {
        a = SET_BIT(b, i);
        printf("%04X, \t", a);
        print_binary(a);
    }

    DEBUG_TIME_LINE("print done");

    return 0;
}
