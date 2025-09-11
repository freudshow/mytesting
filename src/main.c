#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void messagetest(char *name);

int main(int argc, char **argv)
{
    messagetest(argv[0]);
    return 0;
}
