#define _XOPEN_SOURCE
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <uuid/uuid.h>

#include <pwd.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <strings.h>

#include "request.h"
#include "net.h"
#include "utils.h"

extern int flagc;

void 
setClientIP(Request *req, char *str)
{
	char *ip;

	if((ip = malloc((INET6_ADDRSTRLEN + 1) * sizeof(char))) == NULL) {
		perror("malloc error");
	} 
	//bzero(ip, sizeof(ip));
    memset(ip, '\0', sizeof(ip));
	strncpy(ip, str, INET6_ADDRSTRLEN);
	req->clientIP = ip;
}

/*
 * return 0 if it's normal; otherwise return error code
 */
int 
parseRequestLine (char *line, Request *req) 
{
    char *method, *uri, *version;
    char *pch;
    size_t len;   /* the length of pch */
    
    if ((req->requestLine = (char*)malloc(strlen(line)+1)) == NULL)
        return 500;
    strncpy(req->requestLine, line, strlen(line));
    req->requestLine = trimTrailingNewLine(req->requestLine);


    /* parse method */
    pch = strtok(line, " \n");
    if (pch == NULL) {
        return 400;
    }

    len = strlen(pch);
    method = (char *)malloc((len+1)*sizeof(char));
    if (method == NULL) {
        return 500;
    }
    strncpy(method, pch, strlen(pch));
    method[len] = '\0';
     
    /* parse uri */
    pch = strtok(NULL, " ");
    if (pch == NULL) {
        return 400;
    }

    len = strlen(pch);
    uri = (char *)malloc((len+1)*sizeof(char));
    if (uri == NULL) {
        return 500;
    }
    strncpy(uri, pch, strlen(pch));
    uri[len] = '\0';
   
    /* check uri's length */
    if ((strlen(uri) + strlen(method) + 2 + 10) > BUFSIZE) {
        return 400;
    }
     
    /* parse version */
    pch = strtok(NULL, " ");
    if (pch == NULL) {
        return 400;
    }
    len = strlen(pch);
    
    version = (char *)malloc((len+1)*sizeof(char));
    if (version == NULL) {
        return 500;
    }
    strncpy(version, pch, len);
    version[len] = '\0';
    
    req->method = method;
    req->uri = uri;
    req->version = version;
	
	parseURL(req);
   
	if (!isValidURL(req)) {
		return 400;
	}

	decodeRequest(req);

    return 0;
}

int 
parseRequestHeader(char *line, Request *req) 
{
    if (strstr(line, "If-Modified-Since") == line) {
        char *header;
        time_t t;
        char *pch;
        size_t len;
        struct tm tx;
        char *date;

        if((header = (char*)malloc(strlen(line)+1)) == NULL) {
            return 500;
        }
        strcpy(header, line);
        header[strlen(line)] = '\0';

        pch = strtok(header, ":");

        if (pch == NULL) {
            return 400;
        }

        pch = strtok(NULL, "\n");
        if (pch == NULL) {
            return 400;
        }
        pch += 1;
        len = strlen(pch);
        date = (char *)malloc((len+1)*sizeof(char));
        if (date == NULL) {
            return 500;
        }
        strncpy(date, pch, strlen(pch));
        date[len] = '\0';

#if defined(__linux__) || defined(__NetBSD__)
        /* parse date to t */
        if (strptime(date, "%a, %d %b %Y %H:%M:%S %Z", &tx) == NULL) {
            return 400;
        }
#endif
#if defined(__sun__)
        char wday[5];
        char wmon[5];
        int year;
        if((sscanf(date, "%s, %d %s %d %d:%d:%d GMT", wday, &(tx.tm_mday), wmon, &year, &(tx.tm_hour), &(tx.tm_min), &(tx.tm_sec)))<7)
            return 400;
        tx.tm_year = year - 1900;
        if((strcmp(wday, "Sun")) == 0)
            tx.tm_wday = 0;
        if((strcmp(wday, "Mon")) == 0)
            tx.tm_wday = 1;
        if((strcmp(wday, "Tue")) == 0)
            tx.tm_wday = 2;
        if((strcmp(wday, "Wed")) == 0)
            tx.tm_wday = 3;
        if((strcmp(wday, "Thu")) == 0)
            tx.tm_wday = 4;
        if((strcmp(wday, "Fri")) == 0)
            tx.tm_wday = 5;
        if((strcmp(wday, "Sat")) == 0)
            tx.tm_wday = 6;
        if((strcmp(wmon, "Jan")) == 0)
            tx.tm_mon = 0;
        if((strcmp(wmon, "Feb")) == 0)
            tx.tm_mon = 1;
        if((strcmp(wmon, "Mar")) == 0)
            tx.tm_mon = 2;
        if((strcmp(wmon, "Apr")) == 0)
            tx.tm_mon = 3;
        if((strcmp(wmon, "May")) == 0)
            tx.tm_mon = 4;
        if((strcmp(wmon, "Jun")) == 0)
            tx.tm_mon = 5;
        if((strcmp(wmon, "Jul")) == 0)
            tx.tm_mon = 6;
        if((strcmp(wmon, "Aug")) == 0)
            tx.tm_mon = 7;
        if((strcmp(wmon, "Sep")) == 0)
            tx.tm_mon = 8;
        if((strcmp(wmon, "Oct")) == 0)
            tx.tm_mon = 9;
        if((strcmp(wmon, "Nov")) == 0)
            tx.tm_mon = 10;
        if((strcmp(wmon, "Dec")) == 0)
            tx.tm_mon = 11;
#endif


        t = mktime(&tx);
        if (t == -1) {
            return 500;
        }

        req->t = t;
        return 0;
    }else if(strstr(line, ":") != NULL) {
        return 0;
    }else if(strcmp(line, "\x0d\x0a") == 0){
        return 0;
    }else {
        return 400;
    }
}

void 
initialize (Request *req) 
{
    req->method = NULL;
    req->uri = NULL;
	req->path = NULL;
	req->queryString = NULL;
	req->fragment = NULL;
	req->filename = NULL;
    req->requestLine = NULL;
    req->version = NULL;
    req->t = 0;
	req->clientIP = NULL;
}

void 
display (Request *req) 
{
    if (req == NULL)
        return;
    
    if(req->requestLine != NULL)
        printf("whole request line: %s\n", req->requestLine);

    if (req->method != NULL)
        printf("method: %s\n", req->method);
    if (req->uri != NULL)
        printf("uri: %s\n", req->uri);
    if (req->path != NULL)
        printf("path: %s\n", req->path);
    if (req->queryString != NULL)
        printf("query string: %s\n", req->queryString);
    if (req->fragment != NULL)
        printf("fragment: %s\n", req->fragment);
    if (req->filename != NULL)
        printf("filename: %s\n", req->filename);
    if (req->version != NULL)
        printf("version: %s\n", req->version);

    if (req->t != 0)
        printf("since: %lld\n", (long long)req->t);

    if (req->clientIP != NULL)
        printf("clientIP: %s\n", req->clientIP);
}

void 
finalize (Request *req) 
{
    if (req->method != NULL) 
        free(req->method);
    if (req->uri != NULL)
        free(req->uri);
    if (req->path != NULL)
        free(req->path);
    if (req->queryString != NULL)
        free(req->queryString);
    if (req->fragment != NULL)
        free(req->fragment);
    if (req->filename != NULL)
        free(req->filename);
    if (req->requestLine != NULL)
        free(req->requestLine);
    if (req->version != NULL)
        free(req->version);
    if (req->clientIP != NULL)
        free(req->clientIP);
}

/*
 *  if the request is valid, return 0; otherwise, return error code 
 */
int 
isValidRequest(Request *req) 
{
    if ((req->method == NULL) 
            || (req->uri == NULL) || (req->version == NULL) 
            || (req->clientIP == NULL))
        return 400;
    if (strcmp(req->method, "POST") == 0 
            || strcmp(req->method, "PUT") == 0
            || strcmp(req->method, "DELETE") == 0
            || strcmp(req->method, "LINK") == 0
            || strcmp(req->method, "UNLINK") == 0) 
        return 501;
    else {
        if ((strcmp(req->method, "GET") != 0) 
                && (strcmp(req->method, "HEAD") != 0)) {
            return 400;
        }
    }

    if((req->uri)[0] != '/') {
        return 400;
    }

    if(strcmp(req->version, "HTTP/1.0") == 0)
        return 0;

    if (strstr(req->version, "HTTP/") == req->version){
        char *v;
        v = &(req->version)[5];
        if(isdigit(*v) == 0) {
            return 400;
        }
        while(*v != '\0') {
            if(isdigit(*v) == 0)
                break;
            v++;
        }
        if(*v == '\0') {
            return 400;
        }
        if(*v != '.') {
            return 400;
        }
        v++;
        if(isdigit(*v) ==0) {
            return 400;
        }
         while(*v != '\0') {
            if(isdigit(*v) == 0)
                break;
            v++;
        }
        if(*v != '\0') {
            return 400;
        }
        return 505;
    }else {
        return 400;
    }
    return 0;
}

int 
isCGIReq(Request *req) 
{
    const char *uri;

    uri = req->uri;

    if(flagc == 0)
        return 0;

    if ((uri!=NULL) && (uri==strstr(uri, "/cgi-bin")))
        return 1;
    else
        return 0;
}


void 
decodeRequest(Request *req) 
{
	if (req == NULL) {
		return;
	}

	if (req->path != NULL) {
		req->path = decodeURL(req->path);
	}
	
	if (req->queryString != NULL) {
		req->queryString = decodeURL(req->queryString);
	}

	if (req->fragment != NULL) {
		req->fragment = decodeURL(req->fragment);
	}

	if (req->filename != NULL) {
		req->filename = decodeURL(req->filename);
	}
}

