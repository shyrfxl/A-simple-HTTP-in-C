#ifndef REQUEST_H
#define REQUEST_H
#include <time.h>
#include <sys/socket.h>

#define BUFFSIZE 512

int flagd, flagh, flagc;
char *cgiDir;

typedef struct {
    char *requestLine;
    char *method;
    char *uri;
	char *path;
	char *queryString;
	char *fragment;
	char *filename;
    char *version;
	char *clientIP;
    /* for if-modified-header, t==0if no such a header is specified */
    time_t t;   
} Request;

void initialize (Request *);
void display (Request *);
void finalize (Request *);  /* deallocate the space */

int parseRequestLine(char *, Request *);
int parseRequestHeader(char *, Request *);
int findResource (Request *);
int isValidRequest(Request *);
int isCGIReq(Request *);
void sendInvalidRequestError();
void setClientIP(Request *, char *);
void decodeRequest(Request *);
 
#endif
