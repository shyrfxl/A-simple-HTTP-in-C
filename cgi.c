#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cgi.h"
#include "error.h"
#include "http.h"
#include "logging.h"
#include "net.h"
#include "request.h"
#include "response.h"
#include "utils.h"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif

int flagc = 0;
char *cgiDir = NULL;
char *scriptPath = NULL;
int  exectime = 100;

void 
handleCGI(Request *req, int sdconn)
{
	if (flagc == 0) {
		handleError(501, sdconn);
		swsLog(req, 501, 0);
		return;
	}
	initReqMetaVars(sdconn);
	setReqMetaVars(req, sdconn);
	scriptPath = CombineScriptPath(req, sdconn);
	ExecuteScript(scriptPath, sdconn,req);
}


void
initReqMetaVars(int sdconn)
{
	char *env_var[] = {"GATEWAY_INTERFACE", "PATH_INFO", "PATH_TRANSLATED", "QUERY_STRING",
						"REMOTE_ADDR", "REQUEST_METHOD", "SCRIPT_NAME", "SERVER_NAME",
						"SERVER_PORT", "SERVER_PROTOCOL", "SERVER_SOFTWARE" };
	int total_env_var = 11;
	int i;

	for (i = 0; i < total_env_var; i++) {
		unsetReqMetaVar(env_var[i], sdconn);
	}
}

void 
unsetReqMetaVar(char *var, int sdconn)
{
	if (unsetenv(var) == -1) {
		handleError(500, sdconn);
		return;
	}
}

void 
setReqMetaVars(Request *req, int sdconn) 
{
	char *ptr;

	setReqMetaVar("GATEWAY_INTERFACE", "CGI/1.1", sdconn);
	setReqMetaVar("QUERY_STRING", getQueryString(req->uri), sdconn);
	setReqMetaVar("REMOTE_ADDR", trim(req->clientIP), sdconn);
	setReqMetaVar("REQUEST_METHOD", trim(req->method), sdconn);
	setReqMetaVar("SCRIPT_NAME", getScriptName(req->uri, sdconn), sdconn);
	setReqMetaVar("SERVER_PORT", port, sdconn);
	setReqMetaVar("SERVER_SOFTWARE", "SWS-Slytherin/1.0", sdconn);
	setReqMetaVar("SERVER_PROTOCOL", trim(req->version), sdconn);
	
	ptr = getPathInfo(req->uri, sdconn);
	if (ptr != NULL) {
		setReqMetaVar("PATH_INFO", ptr, sdconn);
	}

	ptr = getenv("PATH_INFO");
	if (ptr != NULL) {
		setReqMetaVar("PATH_TRANSLATED", getPathTrans(ptr, sdconn), sdconn);
	}
	
	if (ipAddress == NULL) {
		char serverName[HOST_NAME_MAX + 1];
		if (gethostname(serverName, sizeof(serverName) - 1) == 0) {
			setReqMetaVar("SERVER_NAME", trim(serverName), sdconn);
		} else {
			handleError(500, sdconn);
			return;
		}
	} else {
			setReqMetaVar("SERVER_NAME", trim(ipAddress), sdconn);
	}
}

void 
setReqMetaVar(char *var, char *val, int sdconn)
{
	if (val == NULL){
		return;
	}
	if (setenv(var, val, TRUE) == -1) {
		handleError(500, sdconn);
		return;
	}
}

char *
getPathInfo(char *uri, int sdconn)
{
	char *ptr = NULL;
	char *endp = NULL;
	char *cgiBin = "/cgi-bin/";
	char *pathinfo = "";

	if (strstr(uri, cgiBin) == uri) {
		ptr = uri + strlen(cgiBin);

		ptr = strchr(ptr, '/');
		if (ptr == NULL) { // extra-path empty
			return NULL;
		} 
		endp = strchr(ptr, '?');	// check for query string
		if (endp == NULL) {		// no query string
			if ((pathinfo = malloc((strlen(ptr) + 1) * sizeof(char))) == NULL) {
				handleError(500, sdconn);
				return NULL;
			}
			strncpy(pathinfo, ptr, strlen(ptr)+1);
			pathinfo[strlen(ptr)] = '\0';
			return pathinfo;	
		} 
				
		if ((pathinfo = (char *)malloc((endp - ptr+2) * sizeof(char))) == NULL) {
			handleError(500, sdconn);
			return NULL;
		}
					
		memset(pathinfo, 0 , sizeof(pathinfo));
		strncpy(pathinfo, ptr, endp - ptr+1);
		pathinfo[endp - ptr+1] = '\0';
		return pathinfo;	
	} 

	return pathinfo;
}

char *
getPathTrans(char *pathinfo, int sdconn)
{
	char *pathTrans;
	if (strcmp(pathinfo, "") == 0) {
		return NULL;
	}
	if ((pathTrans = malloc(PATH_MAX * sizeof(char))) == NULL) {
		handleError(500, sdconn);
		return NULL;
	}

	if (chdir(documentRoot) == -1) {
		handleError(500, sdconn);
		return NULL;
	}

	if (getcwd(pathTrans, PATH_MAX) == NULL) {
		handleError(500, sdconn);
		return NULL;
	}

	if (strlen(pathTrans) + strlen(pathinfo) + 1 > PATH_MAX) {
		handleError(500, sdconn);
		return NULL;
	}
	
	strncat(pathTrans, pathinfo, strlen(pathinfo));
	return pathTrans;
}

char *
getScriptName(char *uri, int sdconn)
{
	char *startp = NULL;
	char *endp = NULL;
	char *ptr;
	char *scriptName = NULL;
	char *cgiBin = "/cgi-bin/";

	startp = uri;
	endp = uri + strlen(uri);

	// query string present?
	ptr = strchr(startp, '?');	
	if (ptr != NULL) {
		endp = ptr;
	}

	// extra-path present?
	if (strstr(uri, cgiBin) == uri) {
		ptr = startp + strlen(cgiBin);

		ptr = strchr(ptr, '/');
		if (ptr != NULL) { // extra-path non-empty
			if (ptr < endp) {
				endp = ptr;
			}
		} 
	}

	if ((scriptName = malloc((endp - startp + 1) * sizeof(char))) == NULL) {
		handleError(500, sdconn);
		return NULL;
	}

	memset(scriptName, 0, sizeof(scriptName));
	strncpy(scriptName, startp, endp - startp);
	scriptName[endp - startp] = '\0';
	
	return scriptName;
}

char* CombineScriptPath( Request *req, int sdconn)
{
	char *realPath = NULL, *N_script = NULL;
	char *ptr = NULL ,*endp = NULL;
	char *cgiBin = "/cgi-bin/";
	struct stat cgidir;
	if ((realPath = malloc(PATH_MAX * sizeof(char))) == NULL) {
		handleError(500, sdconn);
		swsLog(req, 500, 0);
		return NULL;
	}
	memset(realPath, 0, PATH_MAX);

	if ((N_script = malloc(PATH_MAX * sizeof(char))) == NULL) {
		handleError(500, sdconn);
		swsLog(req, 500, 0);
		return NULL;
	}
	memset(N_script, 0, PATH_MAX);

	if ((ptr = malloc(PATH_MAX * sizeof(char))) == NULL) {
		handleError(500, sdconn);
		swsLog(req, 500, 0);
		return NULL;
	}
	memset(ptr, 0, PATH_MAX);

	if (chdir(documentRoot) == -1) {
		handleError(500, sdconn);
		swsLog(req, 500, 0);
		return NULL;
	}
	
	if (getcwd(realPath, PATH_MAX) == NULL) {
		handleError(500, sdconn);
		swsLog(req, 500, 0);
		return NULL;
	}
	strncpy(ptr, req->uri, strlen(req->uri));
	if (strstr(ptr, cgiBin) == ptr) {
		ptr = ptr + strlen(cgiBin);
		endp = strchr(ptr, '/');
		if (endp == NULL) { 
			endp = strchr(ptr, '?');	// check for query string
			if (endp == NULL) {		
				strncpy(N_script, ptr, strlen(ptr));
			}
			else
				strncpy(N_script, ptr, endp-ptr);
		} 
		else{
			strncpy(N_script, ptr, endp-ptr);
		}
	}
	else {
		handleError(500, sdconn);
		swsLog(req, 500, 0);
		return NULL;
	}

	if (strlen(realPath) + strlen(cgiDir) + strlen(N_script) + 2 > PATH_MAX) {
		handleError(500, sdconn);
		swsLog(req, 500, 0);
		return NULL;
	}
	strncat( realPath, "/", 1);
	strncat( realPath, cgiDir, strlen(cgiDir));
	if(realPath[strlen(realPath)-1] != '/') {
		strncat( realPath, "/", 1);
	}

	if((stat(realPath, &cgidir))!= 0) {
        	handleError(500, sdconn);
        	swsLog(req, 500, 0);
       	 	return NULL; 
    }

	if(!S_ISDIR(cgidir.st_mode)) {
		handleError(500, sdconn);
        	swsLog(req, 500, 0);
       	 	return NULL;
	}	
	strncat( realPath, N_script, strlen(N_script));
	//printf("%s\n", realPath);
	return realPath;
}

void 
ExecuteScript(char *Filename, int sdconn, Request *req)
{
	int status =0, pid = 0;
	int ret = 0;
	struct stat Execfile;
	char *Buf = NULL;

	if ((Buf = malloc(PATH_MAX * sizeof(char))) == NULL) {
		handleError(500, sdconn);
		swsLog(req, 500, 0);
		return;
	}
	memset(Buf, 0, PATH_MAX);

	if(access(Filename, F_OK) == -1) {
		handleError(404, sdconn);
		swsLog(req, 404, 0);
		return;
	}

	if(access(Filename, X_OK) == -1) {
		handleError(500, sdconn);
		swsLog(req, 500, 0);
		return;
	}

	if((stat(Filename, &Execfile))!= 0) {
		handleError(500, sdconn);
		swsLog(req, 500, 0);
       	return;
    }

	int fd[2];
	if(signal(SIGALRM, exectimeoutHandler) == SIG_ERR) {
		handleError(500, sdconn);
		swsLog(req, 500, 0);
		return;	
	}

	if (pipe(fd)!= 0) {
		handleError(500, sdconn);
		swsLog(req, 500, 0);
		return;	
	}


	if ((pid = fork()) < 0) {     
		handleError(500, sdconn);
		swsLog(req, 500, 0);
		return;
    }

	else if (pid == 0) { // child 
		alarm(exectime);
		close(fd[0]);
		FILE *rfd = NULL;

		if((rfd = popen(Filename, "r")) == NULL) {
			handleError(500, sdconn);
			swsLog(req, 500, 0);
			return;
		}

		while(fgets(Buf, PATH_MAX, rfd)!=NULL) {
			write(fd[1], Buf, strlen(Buf));
		}
		pclose(rfd);
		
	}

	else {  // parent
		while (wait(&status) != pid);       /* wait for completion  */
		if(!WIFEXITED(status)) {
			handleError(500, sdconn);
			swsLog(req, 500, 0);
			return;
     	}

		else if(WEXITSTATUS(status) == 5) {
			handleError(503, sdconn);
			swsLog(req, 503, 0);
			return;	
		}

		close(fd[1]);

		if (strcmp(req->method, "GET") == 0) {
    		response_status(200, sdconn);
			response_header_cgi(sdconn);
			while((ret = read(fd[0], Buf, PATH_MAX))>0) {
				response_write(sdconn, Buf,ret);
				memset(Buf,0, PATH_MAX+1);
			}
			swsLog(req, 200, 0);
		} else if (strcmp(req->method, "HEAD") == 0) {
    		response_status(200, sdconn);
			response_header_cgi(sdconn);
			swsLog(req, 200, 0);
		} else {
			handleError(501, sdconn);
			swsLog(req, 501, 0);
			return;
		}
	}  

	if(signal(SIGALRM, SIG_IGN) == SIG_ERR) {
		handleError(500, sdconn);
		swsLog(req, 500, 0);
		return;	
	}
	return;
}

int 
CheckcgiDir()
{
	char *realPath = NULL;
	DIR *dir;
	if ((realPath = malloc(PATH_MAX * sizeof(char))) == NULL) {
		return 1;
	}
	memset(realPath, 0, PATH_MAX);

	if (chdir(documentRoot) == -1) {
		return 1;
	}
	
	if (getcwd(realPath, PATH_MAX) == NULL) {
		return 1;
	}
	strncat( realPath, "/", 1);
	strncat( realPath, cgiDir, strlen(cgiDir));
	if(realPath[strlen(realPath)-1] != '/') {
		strncat( realPath, "/", 1);
	}
	dir = opendir(realPath);
	if (dir == NULL) {
		fprintf(stderr, "Error in open directory of cgi file.\n\
Please check again\n");
		return 1;
	}
	return 0;

}
void exectimeoutHandler()
{
	printf("exec timeout!\n");
	exit(5);
}
void 
printEnvVars() 
{
	char *env_var[] = {"GATEWAY_INTERFACE", "PATH_INFO", "PATH_TRANSLATED", "QUERY_STRING",
						"REMOTE_ADDR", "REQUEST_METHOD", "SCRIPT_NAME", "SERVER_NAME",
						"SERVER_PORT", "SERVER_PROTOCOL", "SERVER_SOFTWARE" };
	int total_env_var = 11;
	int i;

	printf("Environment variables:\n");	
	for (i = 0; i < total_env_var; i++) {
		printEnvVar(env_var[i]);
	}
}


void 
printEnvVar(const char *var)
{
	char *envVal = NULL;

	envVal = getenv(var);
	if (envVal != NULL) {
		printf("%s: %s\n", var, envVal);
	}
	
}
