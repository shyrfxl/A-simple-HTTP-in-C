#ifndef HTTP_H
#define HTTP_H
#include "request.h"

void handleHTTP(Request *, char*, int);
int resolvePath(char *, char *, char *);
int validatePath(char *, char *);
void magicType (const char *, char *);
#endif
