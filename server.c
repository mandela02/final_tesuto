#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <time.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define BUFF_SIZE 1024 /*max text line length*/
#define SERV_PORT 3000 /*port*/
#define LISTENQ 8      /*maximum number of client connections */

enum mode
{
    PRATICE,
    NEW,
    CHOOSE
};

//1 = waiting, 2 = testing, 3 = finish

typedef struct
{
    int quesID;
    char answer[BUFF_SIZE];
} client_answers;

typedef struct
{
    int *userID;
} user;

typedef struct
{
    int *roomID;
    int *num_of_question;
    int *time_of_test;
    int *status;
    int *num_of_user;
    user list_of_userID[100];
} rooms;

static int *userCurrentID;
unsigned int port = 3306;
static char *unix_socket = NULL;
unsigned int flag = 0;
char recv_answers[BUFF_SIZE];
char send_message[BUFF_SIZE], recv_message[BUFF_SIZE];
int bytes_sent;
int bytes_received;
int scores;
sqlite3 *db;
client_answers answers[100];
static rooms list_of_room[100];
static int *room_base_ID;
static int *active_room_ID;
int number_of_question;
char *file_name_roomlist = "server_room_list.txt";

void printDONE()
{
    printf("--------------DONE--------------\n");
}

void sendFile(int conn_sock, char *file_name)
{

    char buff[BUFSIZ];
    char trash[BUFSIZ];
    int fd;
    int bytes_sent = 0;
    struct stat file_stat;
    ssize_t bytes_received;
    int offset;
    char file_size[256];
    //printf("TRASH_1: %s\n", trash);
    fd = open(file_name, O_RDONLY);

    if (fd == -1)
    {
        fprintf(stderr, "Error opening file --> %s", strerror(errno));
    }
    else
    {
        if (fstat(fd, &file_stat) < 0)
        {
            fprintf(stderr, "Error fstat --> %s", strerror(errno));
        }
        {

            sprintf(file_size, "%d", file_stat.st_size);
            printf("\nFile size : %d\n", file_stat.st_size);
            // Sending file size
            bytes_sent = send(conn_sock, file_size, sizeof(file_size), 0);
            if (bytes_sent <= 0)
            {
                printf("\nConnection closed!\n");
            }
            recv(conn_sock, trash, BUFSIZ - 1, 0);
            printf("message size: %s --- done", trash);
            int fs_block_sz;
            offset = 0;

            while (fs_block_sz = sendfile(conn_sock, fd, &offset, BUFSIZ) > 0)
            {
                printf("sending ...\n");
            }
            printf("Ok File  was Sent!\n");

            bytes_received = recv(conn_sock, buff, BUFSIZ - 1, 0);
            if (bytes_received <= 0)
            {
                printf("\nError!Cannot receive data from sever!\n");
                //break;
            }
            buff[bytes_received] = '\0';
            printf("Reply from server: %s", buff);
        }
    }
    remove(file_name);
    printf("DELETE FILE SUCCESSFUL\n");
    printf("Wait for a new turn\n\n");
}

void create_new_room(int num, int time_test)
{
    printf("step 1\n");
    *room_base_ID = *room_base_ID + 1;
    *active_room_ID = *active_room_ID + 1;
    printf("rome base id: %d\n", *room_base_ID);
    printf("step 2\n");
    *list_of_room[*room_base_ID].roomID = *room_base_ID;
    *list_of_room[*room_base_ID].num_of_question = num;
    *list_of_room[*room_base_ID].time_of_test = time_test;
    *list_of_room[*room_base_ID].status = 1;
    *list_of_room[*room_base_ID].num_of_user = 1;
    list_of_room[*room_base_ID].list_of_userID[0].userID = *userCurrentID; //waiting;
}

void create_roomlist_file_and_send(int conn_sock)
{
    memset(send_message, 0, strlen(send_message));
    //bytes_sent = send(conn_sock, "SUCCESS- ", strlen("SUCCESS- "), 0);
    char trash[BUFF_SIZE];
    if (*room_base_ID == 0)
    {
        printf("There is no active room\n");
        bytes_sent = send(conn_sock, "NOROOM- ", strlen("NOROOM- "), 0);
        recv(conn_sock, trash, BUFSIZ - 1, 0);
        printf("trash_create_room_list : %s", trash);
    }
    else if (*active_room_ID == 0)
    {
        printf("There is no active room\n");
        bytes_sent = send(conn_sock, "NOROOM- ", strlen("NOROOM- "), 0);
        recv(conn_sock, trash, BUFSIZ - 1, 0);
        printf("trash_create_room_list : %s", trash);
    }
    else
    {
        int i;
        FILE *ptr;
        ptr = fopen(file_name_roomlist, "w");
        char payload[BUFF_SIZE];

        for (i = 1; i < *room_base_ID + 1; i++)
        {
            sprintf(payload, "%d\t%d\t%d\t%d\n", *list_of_room[i].roomID, *list_of_room[i].num_of_question, *list_of_room[i].time_of_test, *list_of_room[i].status);
            fputs(payload, ptr);
        }
        fputs("\n", ptr);
        fclose(ptr);
        ptr = fopen(file_name_roomlist, "r");
        sprintf(send_message, "SENDROOMLIST-%d", *room_base_ID);
        bytes_sent = send(conn_sock, send_message, strlen(send_message), 0);
        recv(conn_sock, trash, BUFSIZ - 1, 0);
        sendFile(conn_sock, file_name_roomlist);
        fclose(ptr);
        remove(file_name_roomlist);
    }
}

static int callback_write(void *data, int argc, char **argv, char **azColName)
{
    int i;
    char *file_name = "exam_server.txt";
    //fprintf(stderr, "%s: ", (const char *)data);
    FILE *ptr;
    ptr = fopen(file_name, "a");
    char payload[BUFF_SIZE];

    for (i = 0; i < argc - 1; i++)
    {
        char trash[BUFF_SIZE];
        //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        sprintf(payload, "%s\t", argv[i]);
        fputs(payload, ptr);
    }
    fputs("\n", ptr);
    fclose(ptr);
    return 0;
}

static int callback_judge(void *data, int argc, char **argv, char **azColName)
{
    int i;
    printf("Server's judgement \n");
    printf("user_answer: %s, correct_answer: %s\n", data, argv[argc - 1]);
    if (strcmp(data, argv[argc - 1]) == 0)
    {
        scores++;
    }
    printf("CHECK CALLBACK\n");
    return 0;
}

void prepare_test(int conn_sock)
{
    char *file_name = "exam_server.txt";
    char trash[BUFF_SIZE];
    char *zErrMsg = 0;
    int rc;
    rc = sqlite3_open("test.db", &db);

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return (0);
    }
    else
    {
        fprintf(stderr, "Opened database successfully\n");
    }
    const char *data = "Callback function called";
    char *sql;
    sql = "SELECT * from question";
    printf("%s\n", sql);
    rc = sqlite3_exec(db, sql, callback_write, (void *)data, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }
    sqlite3_close(db);
    bytes_sent = send(conn_sock, "SEND- ", strlen("SEND- "), 0);
    recv(conn_sock, trash, BUFSIZ - 1, 0);
    printf("strash _ prepare _ test :%s", trash);
    sendFile(conn_sock, file_name);
    printDONE();
}

void judge(int conn_sock, int num_of_question)
{
    //memset(answers, 0, strlen(answers));
    memset(send_message, 0, strlen(send_message));

    int i = 0;
    scores = 0;
    char *idAndAnswer[2];
    char *listOfAnswer[num_of_question];

    char *p = strtok(recv_answers, ":");
    while (p != NULL)
    {
        printf("%s\n", p);
        listOfAnswer[i++] = p;
        p = strtok(NULL, ":");
    }
    printf("number of question: %d\n", num_of_question);
    for (i = 0; i < num_of_question; i++)
    {
        int temp = 0;
        char *dump = strtok(listOfAnswer[i], "-");
        while (dump != NULL)
        {
            idAndAnswer[temp++] = dump;
            dump = strtok(NULL, "\0");
        }
        answers[i].quesID = atoi(idAndAnswer[0]);
        strcpy(answers[i].answer, idAndAnswer[1]);
        printf("%d/%s\n", answers[i].quesID, answers[i].answer);
    }

    char *zErrMsg = 0;
    int rc;
    rc = sqlite3_open("test.db", &db);

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return (0);
    }
    else
    {
        fprintf(stderr, "Opened database successfully\n");
    }
    for (i = 0; i < num_of_question; i++)
    {
        printf("begin\n");
        char *data = answers[i].answer;
        int id = answers[i].quesID;
        //printf("data: %s + %d\n", data, id);
        char sql[BUFF_SIZE];
        //         char *sql = "SELECT * from question WHERE id =  1;";
        sprintf(sql, "SELECT * from question WHERE id =  %d;", id);
        //printf("sql: %s\n", sql);

        rc = sqlite3_exec(db, sql, callback_judge, (void *)data, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else
        {
            fprintf(stdout, "Operation done successfully\n");
        }
    }

    sqlite3_close(db);
    printf("score: %d\n", scores);
    sprintf(send_message, "SENDRESULT-%d", scores);
    printf("send score: %s - %d\n", send_message, strlen(send_message));
    send(conn_sock, send_message, strlen(send_message), 0);
    printDONE();
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("error, too many or too few arguments\n");
        printf("Correct format is /.server YourPort\n");
        return 1;
    }

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    int listen_sock, conn_sock; /* file descriptors */

    struct sockaddr_in server; /* server's address information */
    struct sockaddr_in client; /* client's address information */
    int sin_size;

    //Step 1: Construct a TCP socket to listen connection request
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    { /* calls socket() */
        perror("\nError: ");
        return 0;
    }
    //Step 2: Bind address to socket
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[1]));     /* Remember htons() from "Conversions" section? =) */
    server.sin_addr.s_addr = htonl(INADDR_ANY); /* INADDR_ANY puts your IP address automatically */
    if (bind(listen_sock, (struct sockaddr *)&server, sizeof(server)) == -1)
    { /* calls bind() */
        perror("\nError: ");
        return 0;
    }

    //Step 3: Listen request from client
    if (listen(listen_sock, LISTENQ) == -1)
    { /* calls listen() */
        perror("\nError: ");
        return 0;
    }
    //PrintList();
    int pid;
    int i, j;
    //Step 4: Communicate with client"
    printf(" -- BEGIN PROGRESS -- \n");
    room_base_ID = mmap(NULL, sizeof *room_base_ID, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    active_room_ID = mmap(NULL, sizeof *active_room_ID, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *room_base_ID = 0;
    *active_room_ID = 0;

    for (i = 0; i < 100; i++)
    {
        list_of_room[i].roomID = mmap(NULL, sizeof *list_of_room[i].roomID, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        list_of_room[i].num_of_question = mmap(NULL, sizeof *list_of_room[i].num_of_question, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        list_of_room[i].time_of_test = mmap(NULL, sizeof *list_of_room[i].time_of_test, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        list_of_room[i].status = mmap(NULL, sizeof *list_of_room[i].status, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        list_of_room[i].num_of_user = mmap(NULL, sizeof *list_of_room[i].num_of_user, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        for (j = 0; j < 100; j++)
            list_of_room[i].list_of_userID[j].userID = mmap(NULL, sizeof *list_of_room[i].list_of_userID[j].userID, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    }
    userCurrentID = mmap(NULL, sizeof *userCurrentID, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *userCurrentID = 1;
    while (1)
    {
        //accept request
        sin_size = sizeof(struct sockaddr_in);
        if ((conn_sock = accept(listen_sock, (struct sockaddr *)&client, &sin_size)) == -1)
            perror("\nError: ");

        printf("You got a connection from %s - %d\n", inet_ntoa(client.sin_addr), conn_sock); /* prints client's IP */

        pid = fork();
        if (pid == -1)
        {
            close(conn_sock);
            printf("ERROR in new process creation\n");
            exit(1);
        }
        else if (pid > 0)
        { //parent
            close(conn_sock);
            continue;
        }
        else if (pid == 0)
        {
            //kill() gui tin hieu den progress khac
            //child process
            //handleSignal()
            //do whatever you want

            while (1)
            {
                *userCurrentID++;
                printf("-----------BEGIN-------------\n");
                int bytes_received;
                char *roomInformation[2];
                char *roomID;
                memset(recv_message, '\0', strlen(recv_message));
                //memset(roomInformation, '\0', strlen(roomInformation));
                char *request[2];
                int i = 0;

                bytes_received = recv(conn_sock, recv_message, BUFF_SIZE, 0);
                recv_message[bytes_received] = '\0';
                printf("Message: %s.\n", recv_message);

                char *p = strtok(recv_message, "-");
                while (p != NULL)
                {
                    request[i++] = p;
                    p = strtok(NULL, "\0");
                }
                printf("Receiving data ...\n");

                if (strcmp(request[0], "PRACTICE") == 0)
                {
                    printf("Practice start\n");
                    prepare_test(conn_sock);
                    number_of_question = 3;
                    printDONE();
                }
                else if (strcmp(request[0], "NEW") == 0)
                {
                    char trash[BUFSIZ];
                    printf("New test start\n");
                    i = 0;
                    char *dump = strtok(request[1], "-");
                    while (dump != NULL)
                    {
                        roomInformation[i++] = dump;
                        dump = strtok(NULL, "\0");
                    }
                    printf("Number of question: \"%d\". \n", atoi(roomInformation[0]));
                    number_of_question = atoi(roomInformation[0]);
                    printf("Time: \"%d\". \n", atoi(roomInformation[1]));
                    printf("create room\n");
                    create_new_room(atoi(roomInformation[0]), atoi(roomInformation[1]));
                    printf("create done\n");
                    bytes_sent = send(conn_sock, "CREATEROOM- ", strlen("CREATEROOM- "), 0);
                    recv(conn_sock, trash, BUFF_SIZE, 0);
                    printf("trash: %s", trash);
                    printf("number of active room: %d\n", *active_room_ID);
                    printDONE();
                }
                else if (strcmp(request[0], "START") == 0)
                {
                    *active_room_ID = *active_room_ID - 1;
                    printf("number of active room: %d\n", *active_room_ID);
                    bytes_received = recv(conn_sock, recv_message, BUFF_SIZE, 0);
                    recv_message[bytes_received] = '\0';
                    printf("Message: %s.\n", recv_message);
                    prepare_test(conn_sock);
                    *list_of_room[*room_base_ID].status = 2;
                }
                else if (strcmp(request[0], "REQUESTLIST") == 0)
                {
                    printf("Choose room test start\n");
                    create_roomlist_file_and_send(conn_sock);
                    printDONE();
                }
                else if (strcmp(request[0], "CHOOSE") == 0)
                {
                    printf("Choose room test start\n");
                    roomID = request[1];
                    //printf("RoomID: %d - %s\n", atoi(roomID), roomID);

                    for (i = 1; i < *room_base_ID + 1; i++)
                    {
                        printf("room id: %d\n", *list_of_room[i].roomID);
                        if (atoi(roomID) == *list_of_room[i].roomID)
                        {
                            printf("begin test\n");
                            *list_of_room[i].num_of_user++;
                            *list_of_room[i].list_of_userID[*list_of_room[i].num_of_user - 1].userID = *userCurrentID;
                            prepare_test(conn_sock);
                            number_of_question = *list_of_room[i].num_of_question;
                        }

                        else
                        {
                            printf("dead\n");
                        }
                    }
                    printDONE();
                }
                else if (strcmp(request[0], "FINISH") == 0)
                {
                    strcpy(recv_answers, request[1]);
                    recv_answers[strlen(recv_answers) - 1] = '\0';
                    printf("Answers: %s\n", recv_answers);
                    judge(conn_sock, number_of_question);
                    printDONE();
                }
                else if (strcmp(request[0], "QUIT") == 0)
                {
                    printf("Disconnected\n");
                    //kill(pid, SIGTERM);
                    printDONE();
                    break;
                }
                else
                {
                    printf("u r so fucking dead\n");
                    printDONE();
                    //prepare_test(conn_sock);
                }
            }
            break;
            close(conn_sock);
        }
    }
    close(listen_sock);
    return 0;
}