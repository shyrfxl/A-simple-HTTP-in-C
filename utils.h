#ifndef UTILS_H
#define UTILS_H

#include "request.h"

char * trim(char *);
char * trimTrailingNewLine(char *);
char * subStr (char *); 

char * decodeURL(char *);
char getOctetFromEscaped(char, char);
char hexToDec(char);

void parseURL(Request *);
char * getFragment(char *);
char * getQueryString(char *);
char * getPath(char *);
char * getFilename(char *);

int isValidURL(Request *);
int isValidQueryOrFrag(char *);
int isValidPath(char *);
int isPathAbsolute(char *);
int isSegment(char *s, char *);
int isPchar(char *);
int isUnreserved(char);
int isSubDelims(char);
int isPctEncoded(char, char, char);
int isReserved(char);
int isGenDelims(char);


#endif
