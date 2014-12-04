#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"
#include "cgi.h"
#include "error.h"
#include "http.h"
#include "net.h"
#include "request.h"
#include "response.h"
#include "logging.h"

char *documentRoot = NULL;
char *ipAddress = NULL;
char *logFile = NULL;
char *port = "8080";
int sdconn;

void
serverSetup()
{
	int sd;
	if ((sd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		perror("socket() failed");
		exit(1);
	}

	int on = 1;
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0) {
		perror("setsockopt(SO_REUSEADDR) failed");
		exit(1);
	}

	struct sockaddr_in6 serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin6_family = AF_INET6;
	serveraddr.sin6_port = htons(getPort(port));
	if (ipAddress == NULL) {
		serveraddr.sin6_addr = in6addr_any;
	} else {
		if (isIPv4(ipAddress)) {
			const char *newIP = IPv4ToIPv6(ipAddress);
			inet_pton(AF_INET6, newIP, &(serveraddr.sin6_addr));
		} else {
			inet_pton(AF_INET6, ipAddress, &(serveraddr.sin6_addr));
		}
	}
	if (bind(sd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
		perror("bind() failed");
		exit(1);
	}

	if (listen(sd, SOMAXCONN) < 0) {
		perror("listen() failed");
		exit(1);
	}
	printf("listening on port %s: \n", port);
	
    do {
		struct sockaddr_in6 clientaddr;
		socklen_t addrlen = sizeof(clientaddr);
		char str[INET6_ADDRSTRLEN];
		char buf[BUFSIZE];
		char *requestLine, *headerLine;       
		Request req;
        char *pch, *buf1; 

		sdconn = accept(sd, NULL, NULL);
		if (sdconn < 0) {
			perror("accept() failed");
		} else {
			getpeername(sdconn, (struct sockaddr *)&clientaddr, &addrlen);
			if (inet_ntop(AF_INET6, &clientaddr.sin6_addr, str, sizeof(str)) == NULL) {
				handleError(500, sdconn);
				exit(EXIT_FAILURE);
			}
			else {
				pid_t pid;
				pid = fork();
				if (pid < 0) {
					perror("fork() failed");
					handleError(500, sdconn);
					exit(EXIT_FAILURE);
				} 
				else if (pid == 0) {		/* child */
					if (signal(SIGALRM, timeoutHandler) == SIG_ERR) {
						fprintf(stderr, "cannot set signal handler\n");
						handleError(500, sdconn);
						exit(EXIT_FAILURE);
					}
                    alarm(200);		// timeout = 200 sec
					
					requestLine = NULL;
                    int status = 0;
					initialize(&req);
					setClientIP(&req, IPv6ToIPv4(str));
                    
                    /* read all the lines into buffer */
                    //bzero(buf, sizeof(buf));
                    memset(buf, '\0', sizeof(buf));
                    /* while EOF is not in buf and buf is not full*/
                    while ((strlen(buf)<BUFSIZE) && 
                            (strstr(buf, "\x0d\x0a\x0d\x0a")==NULL)) {
                        /* append lines into buf */
                        read(sdconn, buf+strlen(buf), BUFSIZE-strlen(buf));
                    }

					/* Done reading request from client. Reset SIGALRM to ignore the signal */
					if (signal(SIGALRM, SIG_IGN) == SIG_ERR) {
						fprintf(stderr, "cannot set signal handler to SIG_IGN\n");
						handleError(500, sdconn);
						exit(EXIT_FAILURE);
					}

                    //printf("read all: %s", buf);

                    /* abandon the last two characters in buf */
                    buf[strlen(buf)-2] = '\0'; 
                    
                    /* split buf by CR */
                    if (strlen(buf) <= 0) {
						perror("reading stream message");
                        handleError(500, sdconn);
                        exit(EXIT_FAILURE);
                    }
                    pch = subStr(buf);
                    //printf("%s\n", pch);
                    /* parse the request line */
                    if (pch == NULL) {
                        perror("no request line");
                        handleError(500, sdconn);
                        exit(EXIT_FAILURE);
                    }
					if ((requestLine = (char*)malloc(pch-buf+1)) == NULL) {
						perror("Unable to allocate space for requestLine\n");
						handleError(500, sdconn);
                        exit(EXIT_FAILURE);
					}
                    buf1 = pch+2;
					strncpy(requestLine, buf, pch-buf);
					requestLine[pch-buf] = '\0';
					status = parseRequestLine(requestLine, &req);
					//display(&req);
                    if(status != 0){
                        handleError(status, sdconn);
                        exit(EXIT_FAILURE);
                    }
                    free(requestLine);           
                     
                    pch = subStr(buf1);
                    
                    while (pch != NULL) {
                        /* parse it as a header line */
					    if ((headerLine = (char*)malloc(pch-buf1+1)) == NULL) {
				    		perror("Unable to allocate space for headerLien\n");
				    		handleError(500, sdconn);
                            swsLog(&req, 500, 0);
                            exit(EXIT_FAILURE);
				    	}
				    	strncpy(headerLine, buf1, pch-buf1);
				    	headerLine[pch-buf1] = '\0';
                        buf1 = pch+2;
                        status = 0;
                        status = parseRequestHeader(headerLine, &req);
                        if(status != 0){
                            handleError(status, sdconn);
                            swsLog(&req, status, 0);
                            exit(EXIT_FAILURE);
                        }
                        pch = subStr(buf1);
                    }
                    		
					if ((status = isValidRequest(&req)) == 0) {
						if (isCGIReq(&req)) {
							handleCGI(&req, sdconn);
						} else {
							handleHTTP(&req, documentRoot, sdconn);
						}
					} else {
						handleError(status, sdconn);
                        swsLog(&req, status, 0);
					}

					finalize(&req);
					close(sd);
					close(sdconn);
					exit(0);
				}
				else {			/* parent */
					close(sdconn);
				}
			}	
		}
	} while(1);
}



void
printCmdLineArgv()
{
	printf("document Root: %s\n",documentRoot);	
	printf("CGI dir: %s\n",cgiDir);
	printf("IP Address: %s\n", ipAddress);
	printf("Log File: %s\n", logFile);
	printf("Port: %s\n",port);
}

int
isIPv4MappedIPv6(const char *ip)
{
	char *prefix = "::ffff:";
	int n;

	n = strncmp(prefix, ip, strlen(prefix));
	if (n == 0) {
		ip = ip + strlen(prefix);
		return isIPv4(ip);
	} else {
		return FALSE;
	}
}

int 
isIPv4(const char *str) 
{
	int i;
	char *temp;
	char *token;
	char *endptr;
	long val;
	
	temp = (char *)malloc((strlen(str) + 1) * sizeof(char));
	strcpy(temp, str);

	i = 0;
	token = strtok(temp, ".");
	while (token != NULL) {
		i++;
		val = strtol(token, &endptr, 10);
		if (endptr == token) {
			return FALSE;
		}
		if (val < 0 || val > 255) {
			return FALSE;
		}	
		token = strtok(NULL, ".");
	} 

	if (i == 4) {
		return TRUE;	
	} else {
		return FALSE;
	}
}

char *
IPv4ToIPv6(char *ip)
{
	if(isIPv4(ip)) {
		char *prefix = "::ffff:";
		char *ipv4mappedipv6; 

		if ((ipv4mappedipv6 = malloc((strlen(ip) + strlen(prefix) + 1) * sizeof(char))) == NULL) {
			fprintf(stderr, "malloc failed\n");
			exit(1);
		}
		memset(ipv4mappedipv6, 0, sizeof(ipv4mappedipv6));
		strncpy(ipv4mappedipv6, prefix, strlen(prefix) + 1);
		strncat(ipv4mappedipv6, ip, strlen(ip) + 1);
		return ipv4mappedipv6;
	} else {
		return ip;
	}
}

char *
IPv6ToIPv4(char *ip)
{
	if (isIPv4MappedIPv6(ip)) {
		return ip + strlen("::ffff:");
	} else {
		return ip;
	}
}

char *
getDefaultPort()
{
	char *p;

	if ((p = malloc(5 * sizeof(char))) == NULL) {
		perror("malloc error");
		return NULL;
	}

	strncpy(p, "8080", 5);
	p[4] = '\0';
	return p;
}

unsigned short
getPort(char *p)
{
	long port_l;
	char *endptr;

	port_l = strtol(p, &endptr, 10);
	if (port_l != 0 && *p != '\0' && *endptr == '\0') {
		if (port_l > 0 && port_l <= 65535)
			return (unsigned short)port_l;
		else {
			port = getDefaultPort();
			return 8080;
		}
	} else {
		port = getDefaultPort();
		return 8080;
	}
}

void 
timeoutHandler(int sig)
{
	if (signal(SIGALRM, SIG_IGN) == SIG_ERR) {
		fprintf(stderr, "cannot set signal handler to SIG_IGN\n");
	}

	handleError(408, sdconn);
    exit(EXIT_FAILURE);
}
