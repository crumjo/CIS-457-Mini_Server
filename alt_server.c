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
#include <signal.h>

#define FNAME_START 5

struct thread_args
{
    int log_bool;
    int *socket;
    FILE *log_ptr;
};

struct thread_args t_args;

void sigHandler(int sigNum)
{
    fclose(t_args.log_ptr);
    printf("\nExiting Server...\n");
    exit(0);
}

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

int http_message(int clientsocket, int code, int connect, char *d_last_mod, char *con_type, char *file_contents, char *fname)
{
    char *response = (char *)malloc(sizeof(char) * 5000);
    char *code_msg = (char *)malloc(sizeof(char) * 24);
    char *connect_msg = (char *)malloc(sizeof(char) * 16);
    char *print_browser = (char *)malloc(sizeof(char) * 1024);
    char *content_type = (char *)malloc(sizeof(char) * 32);
    char *curr_time = (char *)malloc(sizeof(char) * 128);
    int tmp_len = 0;
    char len[16];
    char *pdf_contents = (char *)malloc(sizeof(char) * 7000);
    FILE *pdf;

    if (code == 200)
    {
        code_msg = "200 OK\r\n";
        print_browser = file_contents;
    }
    else if (code == 304)
    {
        code_msg = "304 Not Modified\r\n";
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

    if (connect)
    {
        connect_msg = "keep-alive\r\n";
    }
    else
    {
        connect_msg = "close\r\n";
        print_browser = file_contents;
    }

    if (strcmp(con_type, "html") == 0)
    {
        content_type = "text/html\r\n";
    }

    else if (strcmp(con_type, "text") == 0)
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

        pdf = fopen(fname, "rb");

        fseek(pdf, 0L, SEEK_END);

        int size = ftell(pdf);
        rewind(pdf);
        sprintf(len, "%d", size);
        fread(pdf_contents, size + 1, 1, pdf);
        //printf("PDF FILE:\n%s\n\n", pdf_contents);
        fclose(pdf);
    }

    get_current_time(curr_time);

    if (strcmp(con_type, "pdf") != 0)
    {
        tmp_len = strlen(print_browser);
        sprintf(len, "%d", tmp_len);
        // printf("> Content Length: %s\n", len);
    }

    strcpy(response, "HTTP/1.1 ");
    strcat(response, code_msg);
    strcat(response, "Date: ");
    strcat(response, curr_time);
    strcat(response, "\r\n");
    strcat(response, "Server: GVSU\r\n");
    strcat(response, "Accept-Ranges: bytes\r\n");
    strcat(response, "Connection: ");
    strcat(response, connect_msg);
    strcat(response, "Content-Type: ");
    strcat(response, con_type);
    strcat(response, "\r\n");

    if (strcmp(con_type, "pdf") == 0)
    {
        strcat(response, "Content-Disposition: inline; filename=");
        strcat(response, fname);
        strcat(response, "\r\n");
    }

    strcat(response, "Content-Length: ");
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

    if (strcmp(con_type, "pdf") != 0)
    {
        strcat(response, print_browser);
        strcat(response, "\r\n");
    }
    else
    {
        strcat(response, pdf_contents);
        strcat(response, "\r\n\n");
    }

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

int read_file_bytes(char *filename, char **buffer, char *dir)
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
        FILE *in_file = fopen(filename, "rb");
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

int get_file_mod_date_jpg(char *path, char *ret_str, char *mod_date)
{
    struct stat attr;
    stat(path, &attr);
    int flag = 1;
    char *format_date = (char *)malloc(512 * sizeof(char));
    char *temp = (char *)malloc(512 * sizeof(char));
    // printf("Last modified time: %s", ctime(&attr.st_mtime));

    memcpy(format_date, ctime(&attr.st_mtime), 3);
    memcpy(temp, ctime(&attr.st_mtime), 3);
    strcat(temp, "\0");
    if (strcmp(strsep(&mod_date, ", "), temp) != 0)
        flag = 0;
    memcpy(format_date + 3, ", ", 2);
    memcpy(format_date + 5, ctime(&attr.st_mtime) + 8, 2);
    memcpy(temp, ctime(&attr.st_mtime) + 8, 2);
    strcat(temp, "\0");
    if (strcmp(strsep(&mod_date, " "), temp) != 0)
        flag = 0;
    memcpy(format_date + 7, " ", 1);
    memcpy(format_date + 8, ctime(&attr.st_mtime) + 4, 3);
    memcpy(temp, ctime(&attr.st_mtime) + 4, 3);
    strcat(temp, "\0");
    if (strcmp(strsep(&mod_date, " "), temp) != 0)
        flag = 0;
    memcpy(format_date + 11, " ", 1);
    memcpy(format_date + 12, ctime(&attr.st_mtime) + 20, 4);
    memcpy(temp, ctime(&attr.st_mtime) + 20, 4);
    strcat(temp, "\0");
    if (strcmp(strsep(&mod_date, " "), temp) != 0)
        flag = 0;
    memcpy(format_date + 16, " ", 1);
    memcpy(format_date + 17, ctime(&attr.st_mtime) + 11, 8);
    memcpy(temp, ctime(&attr.st_mtime) + 11, 8);
    strcat(temp, "\0");
    if (strcmp(strsep(&mod_date, " "), temp) != 0)
        flag = 0;
    memcpy(format_date + 25, " GMT\0", 5);

    strcpy(ret_str, format_date);

    free(format_date);

    return flag;
}

int check_if_mod_since(char *request, char *filename, int client_sock)
{
    // printf("REQUEST:\n%s\n\n", request);
    // printf("\nFILENAME: %s\n", filename);
    char *cp_fname = strdup(filename);
    const char dot = '.';
    char *ext = strrchr(cp_fname, dot);

    if ((strcmp(ext, ".html") != 0) && strcmp(ext, ".txt") != 0)
    {
        return 0;
    }

    // printf("EXT: %s\n", ext);

    while (1)
    {
        char *line = (char *)malloc(512 * sizeof(char));
        char mod_hdr[19];
        
        if (request == NULL)
        {
            return 0;
        }

        strcpy(line, strsep(&request, "\n"));
        printf("POST\n");
        // printf("LINE: %s\n", line);
        if (strcmp(line, "") == 0)
        {
            //printf("BREAK\n\n");
            break;
        }
        else if (strlen(line) > 48)
        {
            memcpy(mod_hdr, line, 18);
            mod_hdr[18] = '\0';
            // printf("HEADER: %s\n", mod_hdr);
            if (strcmp(mod_hdr, "If-Modified-Since:") == 0)
            {
                printf("IF MOD SINCE\n");
                char *date_mod = (char *)malloc(32 * sizeof(char));
                memcpy(date_mod, line + 19, 29);
                //printf("CLIENT SIDE DATE <%s>\n", date_mod);

                /* Get modified date. */
                char *date = (char *)malloc(32 * sizeof(char));
                get_file_mod_date(filename, date);

                //printf("SERVER SIDE DATE <%s>\n", date);
                if (strcmp(date, date_mod) == 0)
                {
                    printf("Date did not change.\n");
                    http_message(client_sock, 304, 1, date, "html", "", "");
                    //free(date_mod);
                    //free(date);
                    return 1;
                }

                //free(date_mod);
                //free(date);
            }
        }
        free(line);
    }
    return 0;
}

void *recv_msg(void *arg)
{
    char *line = (char *)malloc(sizeof(char) * 5000);
    //int clientsocket = *((int *)arg);
    struct thread_args *args = arg;
    int clientsocket = *args->socket;

    while (1)
    {
        recv(clientsocket, line, 5000, 0);

        if (args->log_bool == 1)
        {
            fwrite(line, sizeof(char), sizeof(line) + 1, args->log_ptr);
        }
        else
        {
            printf("> From Client:\n%s\n\n", line);
        }

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
                http_message(clientsocket, 200, 1, "", "html", "<html><body><h1>Connection Successful.</h1></body></html>", "");
                // printf("No file requested.\n");
            }

            /* Requesting a file. */
            else
            {
                /* Get name of file. */
                char get_fname[strlen(startline)];
                memcpy(get_fname, startline + FNAME_START, strlen(startline) - FNAME_START);
                strtok(get_fname, " ");
                printf("FILENAME: %s\n", get_fname);

                /* Check if requesting last modified date. */
                if (access(get_fname, F_OK) != -1)
                {
                    if (check_if_mod_since(line, get_fname, clientsocket))
                    {
                        printf("NO CHANGE TO FILE\n");
                        continue;
                    }
                }

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
                        // printf("HTML REQUEST\n");
                        char *date = (char *)malloc(128 * sizeof(char));
                        get_file_mod_date(get_fname, date);
                        char *file_buffer = (char *)malloc(1024 * sizeof(char));
                        read_file(get_fname, &file_buffer);
                        file_buffer[strlen(file_buffer) + 1] = '\n';
                        // printf("FILE CONTENTS: %s\n", file_buffer);
                        http_message(clientsocket, 200, 1, date, "html", file_buffer, "");
                        free(file_buffer);
                        free(date);
                    }
                    else if (strcmp(ext, ".txt") == 0)
                    {
                        char *date = (char *)malloc(128 * sizeof(char));
                        get_file_mod_date(get_fname, date);
                        char *file_buffer = malloc(sizeof(char));
                        read_file(get_fname, &file_buffer);
                        http_message(clientsocket, 200, 1, date, "text", file_buffer, "");
                        free(file_buffer);
                        free(date);
                    }
                    else if (strcmp(ext, ".jpeg") == 0)
                    {
                        printf("jpeg request\n");
                        char *date = (char *)malloc(128 * sizeof(char));
                        get_file_mod_date_jpg(get_fname, date, startline);
                        char *file_buffer = malloc(sizeof(char) * 5000);
                        char *dir = (char *)malloc(128 * sizeof(char));
                        char *jpgname = (char *)malloc(128 * sizeof(char));
                        strcpy(jpgname, get_fname);
                        strcpy(dir, "./");
                        strcat(dir, strsep(&jpgname, "/"));

                        printf("Dir: %s\njpg: %s\n", dir, jpgname);

                        chdir(dir);
                        // FILE *jpg = fopen(jpgname, "rb");
                        read_file_bytes(jpgname, &file_buffer, dir);
                        chdir(getenv("HOME"));
                        http_message(clientsocket, 200, 1, date, "jpeg", file_buffer, "");
                    }
                    else if (strcmp(ext, ".pdf") == 0)
                    {
                        char *date = (char *)malloc(128 * sizeof(char));
                        get_file_mod_date(get_fname, date);
                        http_message(clientsocket, 200, 1, date, "pdf", "0", get_fname);
                        free(date);
                    }
                    else
                    {
                        http_message(clientsocket, 501, 1, "", "html", "<html><body><h1>File type not supported.</h1></body></html>", "");
                    }
                }

                /* File does not exist */
                else
                {
                    // printf("File not found.\n");
                    http_message(clientsocket, 404, 1, "last_modified", "html", "", "");
                }
            }
        }

        /* Not implemented. */
        else
        {
            http_message(clientsocket, 501, 1, "", "", "", "");
        }
    }

    printf("Exiting Thread...\n");
    return arg;
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigHandler);

    if (argc > 7)
    {
        printf("Enter 3 arguments only:-p, -docroot, and/or -logfile.\n");
        return 1;
    }
    int port = 8080;
    struct thread_args t_args;
    t_args.log_bool = 0;

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
            t_args.log_bool = 1;

            FILE *out_file = fopen(argv[i + 1], "w");
            if (out_file == NULL)
            {
                fprintf(stderr, "File open failed.");
                return -1;
            }
            t_args.log_ptr = out_file;

            printf("Printing to logfile: %s\n", argv[i + 1]);
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

    while (1)
    {
        socklen_t len = sizeof(clientaddr);

        int clientsocket = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
        t_args.socket = &clientsocket;

        if ((status = pthread_create(&recv, NULL, recv_msg, (void *)&t_args)) != 0)
        {
            fprintf(stderr, "Thread create error %d: %s\n", status, strerror(status));
            exit(1);
        }
    }
}
