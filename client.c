#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <time.h>
#include <errno.h>

#define BUFF_SIZE 1024 /*max text line length*/

//struct for question
typedef struct
{
    int quesID;
    char question[BUFF_SIZE];
    char answerA[BUFF_SIZE];
    char answerB[BUFF_SIZE];
    char answerC[BUFF_SIZE];
    char answerD[BUFF_SIZE];
} quiz_t;

//struct for exam room
typedef struct
{
    int roomID;
    int num_of_question;
    int time_of_test;
    int status;
} rooms;

char *file_name_questionlist = "client_exam.txt";
char *file_name_roomlist = "client_room_list.txt";

char send_message[BUFF_SIZE], recv_message[BUFF_SIZE];
char answers[BUFF_SIZE];

int bytes_sent;
int bytes_received;

quiz_t exam_test[100];
rooms list_of_room[100];

//receive file from server
void receiveFile(int client_sock, char *file_name)
{
    int file_size;
    char size[BUFSIZ];
    int i;
    recv(client_sock, size, BUFSIZ, 0);
    file_size = atoi(size);
    send(client_sock, "Success size\n", sizeof("Success size\n"), 0);
    receiveData(client_sock, file_size, file_name);
}

//receive data from server
void receiveData(int client_sock, int file_size, char *file_name)
{
    int bytes_received, bytes_sent;
    char recv_data[BUFSIZ];
    int remain_data = 0;
    FILE *fr = fopen(file_name, "w");
    if (fr == NULL)
    {
        printf("File Cannot be opened file on server.\n");
    }
    else
    {
        remain_data = file_size;

        int fr_block_sz = 0;
        while ((fr_block_sz = recv(client_sock, recv_data, BUFSIZ, 0)) > 0 && (remain_data > 0))
        {
            int write_sz = fwrite(recv_data, sizeof(char), fr_block_sz, fr);
            remain_data -= fr_block_sz;
            //fprintf(stdout, "Receive %d bytes and we hope :- %d bytes\n", fr_block_sz, remain_data);
            if (remain_data == 0)
                break;
        }

        fclose(fr);

    }
}

//read room list file and and save to struct
void readFile_roomList(int num_of_room, char *file_name)
{
    FILE *filein;
    int i;
    int dummy;
    dummy = num_of_room;
    if ((filein = fopen(file_name, "r")) == NULL)
    {
        printf("Cannot open \" exam_client.txt \".\n");
    }
    for (i = 0; i < num_of_room; i++)
    {
        fscanf(filein, "%d\t%d\t%d\t%d", &list_of_room[i].roomID, &list_of_room[i].num_of_question, &list_of_room[i].time_of_test, &list_of_room[i].status);
    }
    fclose(filein);
    remove(file_name);
}

void print_roomList(int num_of_room)
{
    int i;
    for (i = 0; i < num_of_room; i++)
    {
        if (list_of_room[i].status == 1) //waiting
        {
            printf("Room id: %d.\n", list_of_room[i].roomID);
            printf("\tNumber of question: %d \n", list_of_room[i].num_of_question);
            printf("\tTime: %d minute\n", list_of_room[i].time_of_test);
        }
    }
}

//read question list file and save to struct
void readFile_questionlist(int num_of_quest, char *file_name)
{
    FILE *filein;
    if ((filein = fopen(file_name, "r")) == NULL)
    {
        printf("Cannot open \" exam_client.txt \".\n");
    }
    int i;
    for (i = 0; i < num_of_quest; i++)
    {
        fscanf(filein, "%d\t%s\t%s\t%s\t%s\t%s", &exam_test[i].quesID, exam_test[i].question, exam_test[i].answerA, exam_test[i].answerB, exam_test[i].answerC, exam_test[i].answerD);
    }

    fclose(filein);
    remove(file_name_questionlist);
}

void do_the_test(int num_of_quest)
{
    int i;
    // answer of 1 question
    char myAns;
    //question id and answer of 1 question
    char answer[BUFF_SIZE];
    memset(answers, 0, strlen(answers));
    for (i = 0; i < num_of_quest; i++)
    {
        memset(answer, 0, strlen(answer));
        getchar();
        printf("Question %d: %s.\n", i + 1, exam_test[i].question);
        printf("\tAnswer A: %s \n", exam_test[i].answerA);
        printf("\tAnswer B: %s \n", exam_test[i].answerB);
        printf("\tAnswer C: %s \n", exam_test[i].answerC);
        printf("\tAnswer D: %s \n", exam_test[i].answerD);
        printf("Your answer: ");
        scanf("%c", &myAns);
        sprintf(answer, "%d-%c:", exam_test[i].quesID, myAns);
        strcat(answers, answer);
    }
}

//send user answers to server and receive result
void sendAnswer(int client_sock)
{
    memset(send_message, 0, strlen(send_message));
    memset(recv_message, 0, strlen(recv_message));

    strcpy(send_message, "FINISH-");
    strcat(send_message, answers);
    //send answer
    bytes_sent = send(client_sock, send_message, strlen(send_message), 0);

    //receive result
    bytes_received = recv(client_sock, recv_message, BUFF_SIZE - 1, 0);
    recv_message[bytes_received] = '\0';
    char *receive[2];
    int i = 0;
    char *p = strtok(recv_message, "-");
    while (p != NULL)
    {
        receive[i++] = p;
        p = strtok(NULL, "\0");
    }
    printf("YOUR Score: %d\n", atoi(receive[1]));
    remove(file_name_questionlist);
}

//begin test
void begin_test(int client_sock, int num_of_question)
{
    //receive question file
    receiveFile(client_sock, file_name_questionlist);
    //print question to terminal
    readFile_questionlist(num_of_question, file_name_questionlist);
    //
    do_the_test(num_of_question);
    //send answers to server
    sendAnswer(client_sock);
    //delete question list file
}

int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        printf("error, too many or too few arguments\n");
        printf("Correct format is /.client YourId YourPort\n");
        return 1;
    }

    int client_sock;
    struct sockaddr_in server_addr; /* server's address information */

    //Step 1: Construct socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);

    //Step 2: Specify server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    //Step 3: Request to connect server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
    {
        printf("\nError!Can not connect to sever! Client exit immediately! ");
        return 0;
    }

    //Step 4: Communicate with server
    while (1)
    {
        int num_of_quest;
        int time_of_test;
        char commandCode[BUFF_SIZE];

        memset(send_message, '\0', strlen(send_message));
        memset(recv_message, '\0', strlen(recv_message));
        int choice;
        printf("1. Practice\n");
        printf("2. Create a new room\n");
        printf("3. Choose a room\n");
        printf("4. Quit\n");

        do
        {
            printf("Choose (1-4): ");
            scanf("%d", &choice);
            if (choice < 1 || choice > 4)
                printf("Can only choose between 1 and 4\n");
        }

        while (choice < 1 || choice > 4);

        switch (choice)
        {
        case 1:
            strcpy(send_message, "PRACTICE- ");
            bytes_sent = send(client_sock, send_message, strlen(send_message), 0);
            break;
        case 2:
            do
            {
                printf("Please insert number of question (1-30): ");
                scanf("%d", &num_of_quest);
                if (num_of_quest < 1 || num_of_quest > 30)
                    printf("Minimum is 1, maximum is 30\n");
            } while (num_of_quest < 1 || num_of_quest > 30);

            do
            {
                printf("Please insert time (minute): ");
                scanf("%d", &time_of_test);
                if (time_of_test < 1 || time_of_test > 60)
                    printf("Minimum is 1, maximum is 60\n");
            } while (time_of_test < 1 || time_of_test > 60);
            sprintf(send_message, "NEW-%d-%d", num_of_quest, time_of_test);
            bytes_sent = send(client_sock, send_message, strlen(send_message), 0);
            break;
        case 3:
            strcpy(send_message, "REQUESTLIST- ");
            bytes_sent = send(client_sock, send_message, strlen(send_message), 0);
            break;

        case 4:
            strcpy(send_message, "QUIT- ");
            bytes_sent = send(client_sock, send_message, strlen(send_message), 0);
            exit(1);
            break;
        }

        //receive message
        memset(send_message, 0, strlen(send_message));
        memset(recv_message, 0, strlen(recv_message));
        bytes_received = recv(client_sock, recv_message, BUFF_SIZE - 1, 0);
        recv_message[bytes_received] = '\0';
        send(client_sock, "Success message, begin test\n", sizeof("Success message, begin test\n"), 0);
        char *request[2];

        int choose_roomID;
        int i = 0;
        char *p = strtok(recv_message, "-");
        while (p != NULL)
        {
            request[i++] = p;
            p = strtok(NULL, "\0");
        }

        if (strcmp(request[0], "SEND") == 0)
        {
            begin_test(client_sock, 3);
        }
        else if (strcmp(request[0], "NOROOM") == 0)
        {
            printf(" --- NO ROOM AVAILABLE!\n");
        }
        else if (strcmp(request[0], "CREATEROOM") == 0)
        {
            printf(" --- Room created successfully!\n");

            do
            {
                printf("Shall we begin (type \"yes\" to start): ");
                scanf("%s", commandCode);
                if (strcmp(commandCode, "yes") != 0)
                    printf("you can only type \"yes\"\n");
            } while (strcmp(commandCode, "yes") != 0);
            memset(send_message, 0, strlen(send_message));
            sprintf(send_message, "START-%d", atoi(request[1]));
            //strcpy(send_message, "START- ");
            //send start
            send(client_sock, send_message, sizeof(send_message), 0);
            puts(send_message);
            bytes_received = recv(client_sock, recv_message, BUFF_SIZE - 1, 0);
            recv_message[bytes_received] = '\0';
            puts(recv_message); //BEGINTESTROOM

            bytes_received = recv(client_sock, recv_message, BUFF_SIZE - 1, 0);
            recv_message[bytes_received] = '\0';
            printf("message .%s.\n", recv_message);
            send(client_sock, "Success message, begin test\n", sizeof("Success message, begin test\n"), 0);
            begin_test(client_sock, num_of_quest);
        }
        else if (strcmp(request[0], "SENDROOMLIST") == 0)
        {
            int total_room = atoi(request[1]);
            receiveFile(client_sock, file_name_roomlist);
            send(client_sock, "Success message\n", sizeof("Success message\n"), 0);

            readFile_roomList(total_room, file_name_roomlist);
            print_roomList(total_room);
            do
            {
                printf("Choose a room:");
                scanf("%d", &choose_roomID);
                if (choose_roomID < 1 || choose_roomID > total_room)
                    printf("Room not exist in system\n");
            } while (choose_roomID < 1 || choose_roomID > total_room);

            sprintf(send_message, "CHOOSE-%d", choose_roomID);
            bytes_sent = send(client_sock, send_message, strlen(send_message), 0);
            printf("Wait for start signal\n");
            bytes_received = recv(client_sock, recv_message, BUFF_SIZE - 1, 0);
            recv_message[bytes_received] = '\0';
            puts(recv_message); //BEGINTESTROOM
            send(client_sock, "READYTESTROOM- ", sizeof("READYTESTROOM- "), 0);
            if (strcmp(recv_message, "BEGINTESTROOM- ") == 0)
            {
                memset(recv_message, '\0', sizeof(recv_message));
                printf("it's true\n");
                bytes_received = recv(client_sock, recv_message, BUFF_SIZE - 1, 0);
                recv_message[bytes_received] = '\0';
                puts(recv_message);
                send(client_sock, "Success message, begin test\n", sizeof("Success message, begin test\n"), 0);
                begin_test(client_sock, list_of_room[choose_roomID].num_of_question);
            }
            else
            {
                printf("it's False\n");
                /* code */
            }
        }
        else
            printf("Error something\n");
    }

    //Step 4: Close socket
    close(client_sock);
    return 0;
}
