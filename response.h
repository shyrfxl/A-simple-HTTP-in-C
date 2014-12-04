#ifndef RESPONSE_H
#define RESPONSE_H

#define HEADER_MAX 128


void response_write(int client_fd, char *buf, int len);
void response_status(int status, int client_fd);
void response_header(char header[][2][HEADER_MAX], 
        int header_len, int client_fd);
void response_message(int message_fd, int client_fd);
void response_write(int client_fd, char *buf, int len);
void response_header_cgi(int client_fd);

#endif
