#include <string.h>
#include <stdio.h>
#include <stdlib.h>

main(int argc, char const *argv[])
{
    char buf[1024];
    char buf2[1024];
    char *request[2];
    int i = 0;
    strcpy(buf,"START: ");
    char *p = strtok(buf, ":");
    while (p != NULL)
    {
        request[i++] = p;
        p = strtok(NULL, "\0");
    }
    strcpy(buf2, "");
    puts(request[0]);
    puts(request[1]);

    /* code */
    return 0;
}
