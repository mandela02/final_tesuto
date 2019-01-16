#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#define BUFF_SIZE 1024 /*max text line length*/

typedef struct
{
    int quesID;
    char question[BUFF_SIZE];
    char answerA[BUFF_SIZE];
    char answerB[BUFF_SIZE];
    char answerC[BUFF_SIZE];
    char answerD[BUFF_SIZE];
} exam;

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
int bytes_sent;
int bytes_received;
exam exam_test[100];
rooms list_of_room[100];
char answers[BUFF_SIZE];

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
            fprintf(stdout, "Receive %d bytes and we hope :- %d bytes\n", fr_block_sz, remain_data);
            if (remain_data == 0)
                break;
        }

        printf("Ok received from client!\n\n\n");
        fclose(fr);

        bytes_sent = send(client_sock, "Success receiver file\n", sizeof("Success receiver file\n"), 0); /* send to the client welcome message */
        if (bytes_sent <= 0)
        {
            printf("\nConnection closed. doom\n");
        }
        printf("Receiver data done\n");
    }
}

void readFile_roomList(int num_of_room, char *file_name)
{
    FILE *filein;
    int i;
    if ((filein = fopen(file_name, "r")) == NULL)
    {
        printf("Cannot open \" exam_client.txt \".\n");
    }
    for (i = 0; i < num_of_room; i++)
    {
        fscanf(filein, "%d\t%d\t%d\t%d", &list_of_room[i].roomID, &list_of_room[i].num_of_question, &list_of_room[i].time_of_test, &list_of_room[i].status);
        printf("Room id: %d.\n", list_of_room[i].roomID);
        printf("\tNumber of question: %d \n", list_of_room[i].num_of_question);
        printf("\tTime: %d \n", list_of_room[i].time_of_test);
        printf("\tStatus: %s \n", list_of_room[i].status?"active":"deactive");       
    }
}

void readFile_questionlist(int num_of_quest, char *file_name)
{
    FILE *filein;
    if ((filein = fopen(file_name, "r")) == NULL)
    {
        printf("Cannot open \" exam_client.txt \".\n");
    }
    else printf("everthing OK _ 1\n");
    int i;
    char myAns;
    char answer[BUFF_SIZE];
    memset(answers, 0, strlen(answers));

    //irc = fread(exam_test, sizeof(exam), num_of_quest, filein);
    for (i = 0; i < num_of_quest; i++)
    {
        memset(answer, 0, strlen(answer));

        printf("everthing OK _ 2\n");
        getchar();
        fscanf(filein, "%d\t%s\t%s\t%s\t%s\t%s", &exam_test[i].quesID, exam_test[i].question, exam_test[i].answerA, exam_test[i].answerB, exam_test[i].answerC, exam_test[i].answerD);
        printf("Question %d: %s.\n", i + 1, exam_test[i].question);
        printf("\tAnswer A: %s \n", exam_test[i].answerA);
        printf("\tAnswer B: %s \n", exam_test[i].answerB);
        printf("\tAnswer C: %s \n", exam_test[i].answerC);
        printf("\tAnswer D: %s \n", exam_test[i].answerD);
        printf("Your answer: ");
        scanf("%c", &myAns);
        printf("everthing OK _ 3\n");
        sprintf(answer, "%d-%c:", exam_test[i].quesID, myAns);
        printf("everthing OK _ 4\n");
        strcat(answers, answer);
    }
    //printf("myAnswers: %s\n", answers);

    fclose(filein);
}

void sendAnswer(int client_sock)
{
    memset(send_message, 0, strlen(send_message));
    memset(recv_message, 0, strlen(recv_message));

    strcpy(send_message, "FINISH-");
    strcat(send_message, answers);
    printf("message: %s\n", send_message);
    bytes_sent = send(client_sock, send_message, strlen(send_message), 0);
    bytes_received = recv(client_sock, recv_message, BUFF_SIZE - 1, 0);
    recv_message[bytes_received] = '\0';
    printf("Message from server: \"%s\"\n", recv_message);
    char *receive[2];
    int i = 0;
    char *p = strtok(recv_message, "-");
    while (p != NULL)
    {
        receive[i++] = p;
        p = strtok(NULL, "\0");
    }
    printf("Score: %d\n", atoi(receive[1]));
    //free(exam_test);
    //free(answers);
    remove(file_name_questionlist);
}

void begin_test(int client_sock, int num_of_question)
{
    printf("step 1\n");
    receiveFile(client_sock, file_name_questionlist);
    printf("step 2\n");
    readFile_questionlist(num_of_question, file_name_questionlist);
    printf("step 3\n");
    sendAnswer(client_sock);
    printf("step 4\n");
    remove(file_name_questionlist);
}

int main(int argc, char const *argv[])
{
    /* code */

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
        memset(send_message, '\0', strlen(send_message));
        memset(recv_message, '\0', strlen(recv_message));
        int choice;
        printf("1. Practice\n");
        printf("2. Create a new room\n");
        printf("3. Choose a room\n");
        printf("Lua chon: ");
        scanf("%d", &choice);
        switch (choice)
        {
        case 1:
            strcpy(send_message, "PRACTICE- ");
            printf("message send: %s\n", send_message);
            bytes_sent = send(client_sock, send_message, strlen(send_message), 0);

            //TODO: display file to GUI
            break;
        case 2:
            //strcpy(send_message, "NEW-10-120");
            printf("Creat new test room!!!\n");
            printf("Please insert number of question: ");
            scanf("%d", &num_of_quest);
            printf("Please insert time (minute): ");
            scanf("%d", &time_of_test);
            sprintf(send_message, "NEW-%d-%d", num_of_quest, time_of_test);
            printf("message send: %s\n", send_message);
            bytes_sent = send(client_sock, send_message, strlen(send_message), 0);

            break;
        case 3:
            strcpy(send_message, "REQUESTLIST- ");
            printf("message send: %s\n", send_message);
            bytes_sent = send(client_sock, send_message, strlen(send_message), 0);
            break;
        }

        //receive message
        memset(send_message, 0, strlen(send_message));
        memset(recv_message, 0, strlen(recv_message));
        bytes_received = recv(client_sock, recv_message, BUFF_SIZE - 1, 0);
        recv_message[bytes_received] = '\0';
        printf("message received: \"%s\"\n", recv_message);
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
            send(client_sock, "Success message, begin test\n", sizeof("Success message, begin test\n"), 0);
            bytes_received = recv(client_sock, recv_message, BUFF_SIZE - 1, 0);
            recv_message[bytes_received] = '\0';
            printf("message received: \"%s\"\n", recv_message);
            begin_test(client_sock, num_of_quest);
        }
        else if (strcmp(request[0], "SENDROOMLIST") == 0)
        {
            int number_of_room = atoi(request[1]);
            //receive room list
            receiveFile(client_sock, file_name_roomlist);
            //read room list
            readFile_roomList(number_of_room, file_name_roomlist);
            printf(" --- Receive room list successfully\n");
            printf("Choose a room:");
            scanf("%d", &choose_roomID);
            sprintf(send_message, "CHOOSE-%d", choose_roomID);
            printf("message: %s\n", send_message);
            bytes_sent = send(client_sock, send_message, strlen(send_message), 0);
            printf("num of room: %d\n", number_of_room);
            for(i = 0; i < number_of_room; i++)
            {
                if(choose_roomID == list_of_room[i].roomID)
                {
                    printf("Start test %d - %d\n", list_of_room[i].roomID, list_of_room[i].num_of_question);
                    bytes_received = recv(client_sock, recv_message, BUFF_SIZE - 1, 0);
                    recv_message[bytes_received] = '\0';
                    printf("message received: \"%s\"\n", recv_message);
                    send(client_sock, "Success message, begin test\n", sizeof("Success message, begin test\n"), 0);

                    begin_test(client_sock, list_of_room[i].num_of_question);
                }
                else
                {
                    printf("dead daed dead\n");
                }
                
                    //begin_test(client_sock, list_of_room[i].num_of_question);
            }
        }
        else
            printf("U r so fucking dead\n");
    }

    //Step 4: Close socket
    close(client_sock);
    return 0;
}
