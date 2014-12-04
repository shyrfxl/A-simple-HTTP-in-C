#ifndef NET_H
#define NET_H

#define BUFSIZE 10240 
#define TRUE 	1
#define FALSE	0

extern char *documentRoot;
extern char *ipAddress;
extern char *logFile;
extern char *port;

void printCmdLineArgv();
void serverSetup();
int isIPv4(const char *);
int isIPv4MappedIPv6(const char *);
char * IPv6ToIPv4(char *);
char * IPv4ToIPv6(char *);
unsigned short getPort(char *);
char * getDefaultPort();
void timeoutHandler(int);

#endif
