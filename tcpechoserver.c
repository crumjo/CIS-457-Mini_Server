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

int http_message(int clientsocket, char *startline, char *filename, char *response_type)
{
    char *response = (char*) malloc(sizeof(char)*5000);

    strcpy(response, "HTTP/1.1 200 OK\n");
    strcat(response, "Connection: keep-alive");
    strcat(response, "Date: Sun, 18 Oct 2009 08:56:53 GMT\n");
    strcat(response, "Last-Modified: Sat, 20 Nov 2004 07:16:26 GMT\n");
    strcat(response, "Content-Length: 44\n");
    strcat(response, "Content-Type: text/html\n\n");
    strcat(response, "<html><body><h1>It works!</h1></body></html>");

    send(clientsocket, response, 5000, 0);
    return 1;
}


void *recv_msg(void *arg)
{
    char *line = (char*) malloc(sizeof(char)*5000);
    int clientsocket = *((int *)arg);
    while (1)
    {
        recv(clientsocket, line, 5000, 0);
        
        printf("\n%s\n", line);
        
        char startline [100];
        strcpy(startline, strsep(&line, "\n"));

        char cmd [4];
        memcpy(&cmd, &startline, 3);
        cmd[3] = '\0';

        char *filename = (char*) malloc(sizeof(char)*100);

        
        if (strcmp(cmd, "GET") != 0)
        {
            http_message(clientsocket, startline, "", "501");
        }
        else if (fopen(filename, "rb") < 0)
        {
            http_message(clientsocket, startline, filename, "404");
        }
        else
        {
            http_message(clientsocket, startline, filename, "200");
        }
    }
    printf("exiting thread\n");
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
    while(1);
}