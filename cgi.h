#ifndef CGI_H
#define CGI_H
#include "request.h"

extern int flagc;
extern char *cgiDir;

void handleCGI(Request *, int);
void initReqMetaVars(int);
void setReqMetaVars(Request *, int); 
void setReqMetaVar(char *, char *, int);
void unsetReqMetaVar(char *, int);
void printEnvVar(const char *);
void ExecuteScript(char *Filename, int sdconn, Request *req);
void printEnvVars(); 
char* CombineScriptPath( Request *req, int sdconn);
char * getPathInfo(char *, int);
char * getPathTrans(char *, int);
char * getScriptName(char *, int);
int CheckcgiDir();
void exectimeoutHandler();

#endif
