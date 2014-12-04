#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "response.h"


void 
response_write(int client_fd, char *buf, int len)
{
    if (write(client_fd, buf, len) < 0) {
        fprintf(stderr, "Unable to write to client: %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void 
response_status(int status, int client_fd)
{
    char str_status[6][10][128] = {
        {},
        {},
        {"OK", "Created", "Accepted", "", "No Content"},
        {"", "Moved Permanently", "Moved Temporarily", "", "Not Modified"},
        {"Bad Request", "Unauthorized", "", "Forbidden", "Not Found",
         "", "", "", "Request Timeout"},
        {"Internal Server Error", "Not Implemented", "Bad Gateway",
         "Service Unavailable", "", "Version Not Supported"}};
    char str_status_line[256];

    sprintf(str_status_line, "HTTP/1.0 %d %s\n", status,
            str_status[status / 100][status % 100]);
    response_write(client_fd, str_status_line, strlen(str_status_line));
}

void 
response_header(char header[][2][HEADER_MAX], 
        int header_len, int client_fd)
{
    char date_and_server[512];
    char str_date[256];
    time_t _time;
    int i;

    _time = time(NULL);
    strftime(str_date, sizeof(str_date), "%a, %d %b %Y %H:%M:%S",
            gmtime(&_time));
    sprintf(date_and_server, "Date: %s GMT\nServer: SWS/1.0 (slytherin)\n",
            str_date);
    response_write(client_fd, date_and_server, strlen(date_and_server));
    
    for (i = 0; i < header_len; ++i) {
        response_write(client_fd, header[i][0], strlen(header[i][0]));
        response_write(client_fd, ": ", 2);
        response_write(client_fd, header[i][1], strlen(header[i][1]));
        response_write(client_fd, "\n", 1);
    }
    response_write(client_fd, "\n", 1);
}

void 
response_message(int message_fd, int client_fd)
{
    char buf[1024];
    int n;
    while (message_fd >= 0) {
        n = read(message_fd, buf, 1024);
        if (n < 0) {
            fprintf(stderr, "Unable to read from client: %s\n",
                    strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (n == 0)
            break;
        response_write(client_fd, buf, n);
    }
}


void 
response_header_cgi(int client_fd)
{
    char date_and_server[512];
    char str_date[256];
    time_t _time;

    _time = time(NULL);
    strftime(str_date, sizeof(str_date), "%a, %d %b %Y %H:%M:%S",
            gmtime(&_time));
    sprintf(date_and_server, "Date: %s GMT\nServer: SWS/1.0 (slytherin)\n",
            str_date);
    response_write(client_fd, date_and_server, strlen(date_and_server));
    
}
