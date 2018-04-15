#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#define FNAME_START 5


int http_message(int clientsocket, int code, int connect, char *d_last_mod, char *con_type)
{
    char *response = (char *)malloc(sizeof(char) * 5000);
    char *code_msg = (char *)malloc(sizeof(char) * 24);
    char *connect_msg = (char *)malloc(sizeof(char) * 16);
    char *print_browser = (char *)malloc(sizeof(char) * 128);
    int tmp_len = 0;

    if (code == 200)
    {
        code_msg = "200 OK\r\n";
        
        print_browser = "<html><body><h1>It works!</h1></body></html>\r\n";
    }
    else if (code == 404)
    {
        code_msg = "404 Not Found\r\n";
        print_browser = "<html><body><h1>404 Not Found.</h1></body></html>\r\n";
    }
    else if (code == 501)
    {
        code_msg = "501 Not Implemented\r\n";
        print_browser = "<html><body><h1>501 Not Implemented.</h1></body></html>\r\n";
    }

    char len[3];
    tmp_len = strlen(print_browser);
    sprintf(len, "%d", tmp_len);
    //printf("<<<Length: %s>>>\n\n", len);

    if (connect == 1)
    {
        connect_msg = "keep-alive\r\n";
    }
    else
    {
        connect_msg = "close\r\n";
    }

    strcpy(response, "HTTP/1.1 ");
    strcat(response, code_msg);
    strcat(response, "Connection: ");
    strcat(response, connect_msg);

    time_t t;

    //these need to be replaced later
    strcat(response, "Last-Modified: Sat, 20 Nov 2004 07:16:26 GMT\r\n");
    strcat(response, "Date: Sun, 18 Oct 2009 08:56:53 GMT\r\n");
    strcat(response, "Server: TristanJoes\r\n");
    strcat(response, "Accept-range: bytes\r\n");
    strcat(response, "Content-Type: text/html\r\n");
    strcat(response, "Content-Length: ");
    strcat(response, len);
    strcat(response, "\r\n");

    //update this based on file extension: text/html, image/jpeg, text/plain, application/pdf
    strcat(response, "Content-Type: text/html\n\n");

    strcat(response, print_browser);

    printf("Message:\n-----------------\n%s\n-----------------\n", response);

    send(clientsocket, response, 5000, 0);

    return tmp_len;
}

void *recv_msg(void *arg)
{
    char *line = (char *)malloc(sizeof(char) * 5000);
    int clientsocket = *((int *)arg);
    while (1)
    {
        recv(clientsocket, line, 5000, 0);

        printf("\n%s\n", line);

        char startline[100];
        strcpy(startline, strsep(&line, "\n"));

        char cmd[4];
        memcpy(&cmd, &startline, 3);
        cmd[3] = '\0';


        if (strcmp(cmd, "GET") == 0 && strcmp(startline, "") != 0)
        {
            /* Check if requesting a file or not. */
            char file_check[2];
            memcpy(file_check, startline + FNAME_START, 1);

            if (strcmp(file_check, " ") == 0)
            {
                //http_message(clientsocket, startline, filename, "200");
                http_message(clientsocket, 200, 1, "fixme", "fixme");
                printf("No file requested.\n");
            }

            /* Get name of file. */
            char get_fname[strlen(startline)];
            memcpy(get_fname, startline + FNAME_START, strlen(startline) - FNAME_START);
            strtok(get_fname, " ");

            /* Check if file exists. */
            if (access(get_fname, F_OK) != -1)
            {
                /* File exists. */
                printf("File found.\n");
                //http_message(clientsocket, startline, get_fname, "200");
                http_message(clientsocket, 200, 1, "fixme", "fixme");
            }

            /* File does not exist */
            else
            {
                printf("File not found.\n");
                //http_message(clientsocket, startline, filename, "404");
                http_message(clientsocket, 404, 1, "fixme", "fixme");
            }
        }

        /* Not implemented. */
        else
        {
            http_message(clientsocket, 501, 1, "fixme", "fixme");
        }
    }
    printf("exiting thread\n");
    return arg;
}

int main(int argc, char **argv)
{
    if (argc > 7)
    {
        printf("Enter 3 arguments only:-p, -docroot, and/or -logfile.\n");
        return 1;
    }

    int port = 8080;
    int docroot_spec = 0; //check if docroot has been specified, can use in 'if' statement when choosing docroot
    char *dir = (char *)malloc(128 * sizeof(char)); //no free for this yet
    FILE *log_file;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
        {
            port = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-docroot") == 0)
        {
            dir = argv[i + 1];
            docroot_spec = 1;
            printf("Using directory: %s\n", dir);
        }
        else if (strcmp(argv[i], "-logfile") == 0)
        {
            log_file = fopen(argv[i + 1], "w");
            printf("Printing to logfile %s\n", argv[i + 1]);      
        }
    }

    printf("Listening on port %d\n", port);

    int status;
    //char cp_num[16];
    pthread_t recv;

    // printf("Enter a port number: ");
    // fgets(cp_num, 16, stdin);
    // port = atoi(cp_num);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serveraddr, clientaddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(sockfd, 10);
    while (1)
    {
        socklen_t len = sizeof(clientaddr);

        int clientsocket = accept(sockfd, (struct sockaddr *)&clientaddr, &len);

        if ((status = pthread_create(&recv, NULL, recv_msg, &clientsocket)) != 0)
        {
            fprintf(stderr, "Thread create error %d: %s\n", status, strerror(status));
            exit(1);
        }
    }
}