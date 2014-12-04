/*
 *  logging.c  do the basic logging for the web server.
 *
 *
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "logging.h"

#define LOG_MAX 4096

extern int flagl;
extern int flagd;
extern char *logFile;

void swsLog(Request *req, int status, unsigned ResponseLength){
    if(flagd == 1 || flagl == 1){
        char message[LOG_MAX];
        char str_date[200];
        time_t tn = time(0);

        strftime(str_date, 200, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&tn));
        sprintf(message, "%s %s \"%s\" %d %d\n", req->clientIP, str_date, 
                    req->requestLine, status, ResponseLength);
        if(flagd ==1)
            printf("%s", message);
        else{
            int file_fd;

            if((file_fd = open(logFile, O_WRONLY|O_APPEND|O_CREAT, 
                        S_IRWXU|S_IRWXG|S_IROTH)) < 0){
                fprintf(stderr, "%s: %s\n", logFile, strerror(errno));
                return;
            }
            write(file_fd, message, strlen(message));
            close(file_fd);
            return;
        }
    }
}

