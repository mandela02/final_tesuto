#include<stdio.h>
#include<string.h>

main(int argc, char const *argv[])
{
    char *file_name_roomlist = "server_room_list.txt";
    char file_name[1024];
    strcpy(file_name, file_name_roomlist);
    char *data = "3";
    strcat(file_name, data);
    puts(file_name);
    
    return 0;
}
