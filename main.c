/*	A simple web server
 *	Author	: Team Slytherin
 *	Date	: November 3, 2014
 *	Version	: 1.0
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "net.h"
#include "cgi.h"
#include "request.h"
#include "daemon.h"

int flagd, flagh, flagl;
char *programName = NULL;
void Usage();
void Usagehelp();
void daemonlize(); 

int
main(int argc, char *argv[])
{
	int opt;

	programName = argv[0];

	while((opt = getopt(argc, argv, "dhc:i:l:p:")) != -1) {
		switch(opt) {
			case 'd':
				flagd = 1;
				break;
			case 'h':
				flagh = 1;
				Usagehelp();
				break;
			case 'c':
				flagc = 1;
				cgiDir = optarg;
				break;
			case 'i':
				ipAddress = optarg;
				break;
			case 'l':
				flagl = 1;
				logFile = optarg;
				break;
			case 'p':
				port = optarg;
				break;
			default:
				Usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1) {
		Usage();
	}
	documentRoot = argv[0];

	if (flagc == 1) {
		if(CheckcgiDir())
		{
			exit(EXIT_FAILURE);
		}
	}

	if (flagd != 1) 
        daemonlize();
	serverSetup();

	exit(0);
}


void 
Usage()
{
	fprintf(stderr, "usage: %s [-dh] [-c dir][-i address][-l file]\
[-p port]dir  (more details by option \'h\')\n", programName);
	exit(EXIT_FAILURE );	
}

void 
Usagehelp()
{
	fprintf(stdout,"usage: %s <cdhilp> dir\n \
	-c dir\t\t Allow execution of CGIs from the given directory.\n\
	-d    \t\t Enter debugging mode. That is, do not daemonize, only\
 accept one connection at a time and enable logging to\
 stdout.\n\
	-h \t\t Print a short usage summary and exit.\n\
	-i address\t Bind to the given IPv4 or IPv6 address. If not\
 provided,sws will listen on all IPv4 and IPv6 addresses on\
 this host.\n\
	-l file\t\t Log all requests to the given file.\n\
	-p port\t\t Listen on the given port. If not provide,sws will\
 listen on port 8080.\n",programName);
	exit(EXIT_SUCCESS);
}

