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

struct http_response_header
{
    char start[100];
    char date[100];

};


void *recv_msg(void *arg)
{
    char *line = (char*) malloc(sizeof(char)*5000);
    int clientsocket = *((int *)arg);
    struct http_response_header *response = (struct http_response_header*)malloc(sizeof(struct http_response_header));

    while (1)
    {
        line = "";

        if (recv(clientsocket, line, 5000, 0) < 0)
        {
            break;
        };
        printf("\n%s\n", line);
        
        char startline [5000];
        strcpy(startline, strsep(&line, "\n"));
        printf("%s\n", startline);
        
        char cmd [4];
        memcpy(&cmd, &startline, 3);
        cmd[3] = '\0';

        
        if (strcmp(cmd, "GET") != 0)
        {
            //SEND 501 MESSAGE BACK TO CLIENT
        }
        else
        {

        }
    }
    free(line);
    return arg;
}

int main(int argc, char **argv)
{
    int port, status;
    char cp_num[16];
    pthread_t recv;

    printf("Enter a port number: ");
    fgets(cp_num, 16, stdin);
    port = atoi(cp_num);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serveraddr, clientaddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(sockfd, 10);

    socklen_t len = sizeof(clientaddr);

    int clientsocket = accept(sockfd, (struct sockaddr *)&clientaddr, &len);

    if ((status = pthread_create(&recv, NULL, recv_msg, &clientsocket)) != 0)
    {
        fprintf(stderr, "Thread create error %d: %s\n", status, strerror(status));
        exit(1);
    }

    while (1)
    {
        printf("Enter a line: ");
        char line[5000];
        fgets(line, 5000, stdin);
        send(clientsocket, line, strlen(line) + 1, 0);
    }

    close(clientsocket);
}