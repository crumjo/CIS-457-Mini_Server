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
#include <sys/stat.h>

#define FNAME_START 5

void get_current_time(char *ret_str)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    char *format_date = (char *)malloc(512 * sizeof(char));

    memcpy(format_date, asctime(timeinfo), 3);
    memcpy(format_date + 3, ", ", 2);
    memcpy(format_date + 5, asctime(timeinfo) + 8, 2);
    memcpy(format_date + 7, " ", 1);
    memcpy(format_date + 8, asctime(timeinfo) + 4, 3);
    memcpy(format_date + 11, " ", 1);
    memcpy(format_date + 12, asctime(timeinfo) + 20, 4);
    memcpy(format_date + 16, " ", 1);
    memcpy(format_date + 17, asctime(timeinfo) + 11, 8);
    memcpy(format_date + 25, " GMT\0", 5);

    strcpy(ret_str, format_date);

    free(format_date);
}

int http_message(int clientsocket, int code, int connect, char *d_last_mod, char *con_type, char *file_contents)
{
    char *response = (char *)malloc(sizeof(char) * 5000);
    char *code_msg = (char *)malloc(sizeof(char) * 24);
    char *connect_msg = (char *)malloc(sizeof(char) * 16);
    char *print_browser = (char *)malloc(sizeof(char) * 1024);
    char *content_type = (char *)malloc(sizeof(char) * 24);
    char *curr_time = (char *)malloc(sizeof(char) * 128);
    int tmp_len = 0;

    if (code == 200)
    {
        code_msg = "200 OK\r\n";
        print_browser = file_contents;
    }
    else if (code == 404)
    {
        code_msg = "404 Not Found\r\n";
        print_browser = "<html><body><h1>404 Not Found.</h1></body></html>";
    }
    else if (code == 501)
    {
        code_msg = "501 Not Implemented\r\n";
        print_browser = "<html><body><h1>501 Not Implemented.</h1></body></html>";
    }

    char len[6];
    tmp_len = strlen(print_browser);
    sprintf(len, "%d", tmp_len);
    // printf("> Content Length: %s\n", len);

    if (connect)
    {
        connect_msg = "keep-alive\r\n";
    }
    else
    {
        connect_msg = "close\r\n";
    }

    if (strcmp(con_type, "html") == 0)
    {
        content_type = "text/html\r\n";
    }

    else if (strcmp(con_type, "text"))
    {
        content_type = "text/plain\r\n";
    }

    else if (strcmp(con_type, "jpeg") == 0)
    {
        content_type = "image/jpeg\r\n";
    }

    else if (strcmp(con_type, "pdf") == 0)
    {
        content_type = "application/pdf\r\n";
    }

    get_current_time(curr_time);

    strcpy(response, "HTTP/1.1 ");
    strcat(response, code_msg);

    //these need to be replaced later
    strcat(response, "Date: ");
    strcat(response, curr_time);
    strcat(response, "\r\n");
    strcat(response, "Server: GVSU\r\n");
    strcat(response, "Accept-Ranges: bytes\r\n");
    strcat(response, "Connection: ");
    strcat(response, connect_msg);
    strcat(response, "Content-Type: ");
    strcat(response, con_type);
    strcat(response, "\r\nContent-Length: ");
    strcat(response, len);
    strcat(response, "\r\n");

    if (strcmp(d_last_mod, "") != 0)
    {
        strcat(response, "Last-Modified: ");
        strcat(response, d_last_mod);
        strcat(response, "\r\n\n");
    }
    else
    {
        strcat(response, "\n");
    }

    strcat(response, print_browser);
    strcat(response, "\r\n");

    printf("> Response:\n%s\n\n", response);

    send(clientsocket, response, 7000, 0);

    return tmp_len;
}

int read_file(char *filename, char **buffer)
{

    /** The number of elements in the file. */
    int size = 0;

    if (access(filename, F_OK) == -1)
    {
        fprintf(stderr, "\nThe file '%s' cannot be found "
                        "or does not exist.\n\n",
                filename);
        return -1;
    }
    else
    {
        /** Calculate file size, provided by Professor Woodring */
        struct stat st;
        stat(filename, &st);
        size = st.st_size;

        **buffer = (char)malloc(size * sizeof(char));
        if (buffer == NULL)
        {
            fprintf(stderr, "\nMemory allocation failed.\n");
            return -1;
        }

        /** Open the file in read mode. */
        FILE *in_file = fopen(filename, "r");
        if (in_file == NULL)
        {
            fprintf(stderr, "File open failed.");
            fclose(in_file);
            return -1;
        }

        fread(*buffer, sizeof(char), size, in_file);
        fclose(in_file);
    }

    return size;
}

void get_file_mod_date(char *path, char *ret_str)
{
    struct stat attr;
    stat(path, &attr);

    char *format_date = (char *)malloc(512 * sizeof(char));
    // printf("Last modified time: %s", ctime(&attr.st_mtime));

    memcpy(format_date, ctime(&attr.st_mtime), 3);
    memcpy(format_date + 3, ", ", 2);
    memcpy(format_date + 5, ctime(&attr.st_mtime) + 8, 2);
    memcpy(format_date + 7, " ", 1);
    memcpy(format_date + 8, ctime(&attr.st_mtime) + 4, 3);
    memcpy(format_date + 11, " ", 1);
    memcpy(format_date + 12, ctime(&attr.st_mtime) + 20, 4);
    memcpy(format_date + 16, " ", 1);
    memcpy(format_date + 17, ctime(&attr.st_mtime) + 11, 8);
    memcpy(format_date + 25, " GMT\0", 5);

    strcpy(ret_str, format_date);

    free(format_date);
}

void *recv_msg(void *arg)
{
    char *line = (char *)malloc(sizeof(char) * 5000);
    int clientsocket = *((int *)arg);

    while (1)
    {
        recv(clientsocket, line, 5000, 0);

        printf("> From Client:\n%s\n\n", line);

        char startline[100];
        strcpy(startline, strsep(&line, "\n"));

        char cmd[4];
        memcpy(&cmd, &startline, 3);
        cmd[3] = '\0';

        if (strcmp(cmd, "GET") == 0)
        {
            /* Check if requesting a file or not. */
            char file_check[2];
            memcpy(file_check, startline + FNAME_START, 1);

            if (strcmp(file_check, " ") == 0)
            {
                http_message(clientsocket, 200, 1, "", "html", "");
                // printf("No file requested.\n");
            }

            /* Requesting a file. */
            else
            {
                /* Get name of file. */
                char get_fname[strlen(startline)];
                memcpy(get_fname, startline + FNAME_START, strlen(startline) - FNAME_START);
                strtok(get_fname, " ");

                /* Check if file exists. */
                if (access(get_fname, F_OK) != -1)
                {
                    /* File exists. */
                    // printf("File found.\n");

                    const char dot = '.';
                    char *ext = strrchr(get_fname, dot);
                    // printf("Extension: %s\n\n", ext);

                    if (strcmp(ext, ".html") == 0)
                    {
                        char *date = (char *)malloc(128 * sizeof(char));
                        get_file_mod_date(get_fname, date);
                        char *file_buffer = (char *)malloc(1024 * sizeof(char));
                        read_file(get_fname, &file_buffer);
                        file_buffer[strlen(file_buffer) + 1] = '\n';
                        printf("FILE CONTENTS: %s\n", file_buffer);
                        http_message(clientsocket, 200, 1, date, "html", file_buffer);
                        free(file_buffer);
                        free(date);
                    }
                    else if (strcmp(ext, ".txt") == 0)
                    {
                        char *date = (char *)malloc(128 * sizeof(char));
                        get_file_mod_date(get_fname, date);
                        char *file_buffer = malloc(sizeof(char));
                        read_file(get_fname, &file_buffer);
                        http_message(clientsocket, 200, 1, date, "text", file_buffer);
                        free(file_buffer);
                    }
                    else if (strcmp(ext, ".jpeg") == 0)
                    {
                    }
                    else if (strcmp(ext, ".pdf") == 0)
                    {
                    }
                    else
                    {
                        http_message(clientsocket, 501, 1, "", "html", "<html><body><h1>File type not supported.</h1></body></html>");
                    }
                }

                /* File does not exist */
                else
                {
                    // printf("File not found.\n");
                    http_message(clientsocket, 404, 1, "last_modified", "html", "");
                }
            }
        }

        /* Not implemented. */
        else
        {
            http_message(clientsocket, 501, 1, "", "", "");
        }
    }

    printf("Exiting Thread...\n");
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
    FILE *log_file;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
        {
            port = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-docroot") == 0)
        {
            chdir(argv[i + 1]);
            char cwd[1024];
            getcwd(cwd, sizeof(cwd));
            printf("Using directory: %s\n", cwd);
        }
        else if (strcmp(argv[i], "-logfile") == 0)
        {
            log_file = fopen(argv[i + 1], "w");
            printf("Printing to logfile %s\n", argv[i + 1]);
        }
    }

    printf("Listening on port %d\n", port);

    int status;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    pthread_t recv;

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
        ;
}
