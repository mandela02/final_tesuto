
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
static int *global_var;
main(int argc, char const *argv[])
{
    char buf[1024];
    char buf2[1024];
    char *request[2];
    int i = 0;
    *global_var = 1;
    printf("value: %d", global_var);
    /* strcpy(buf,"START: ");
    char *p = strtok(buf, ":");
    while (p != NULL)
    {
        request[i++] = p;
        p = strtok(NULL, "\0");
    }
    strcpy(buf2, "");
    puts(request[0]);
    puts(request[1]);

     */
    return 0;
}
