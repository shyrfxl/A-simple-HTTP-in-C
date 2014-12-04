#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

char *
trim(char *str)
{
	char *endp;

	if (str == NULL) {
		return NULL;
	}

	if (*str == '\0') { 	// empty string
		return str;
	}

	// trim leading space
	while (isspace((int)*str)) {
		str++;
	}
	
	if (*str == '\0') { 	// all space string
		return str;
	}

	// trim trailing space
	endp = str + strlen(str) - 1;
	while (endp > str && isspace((int)*endp)) {
		endp--;
	}
	
	*(endp + 1) = '\0';
	return str;
}

char *
trimTrailingNewLine(char *str)
{
    char *endp;
    endp = str + strlen(str)-1;
    while(endp > str && (*endp == '\r' || *endp == '\n')) {
        *endp = '\0';
        endp--;
    }

    return str;
}

char *subStr (char *str) {
    int i;
    char *pch;

    i = 1;
    pch = NULL;
    while (i < strlen(str)) {
        if ((str[i-1]==13) && (str[i]==10)) {
            pch = str+i-1;
            break;
        }
        i++;
    }

    return pch;
}

void 
parseURL(Request *req)
{
	char *url;

	if (req->uri == NULL) {
		return;
	}

	if ((url = malloc((strlen(req->uri) + 1) * sizeof(char))) == NULL) {
		perror("malloc");
		return;
	}

	strncpy(url, req->uri, strlen(req->uri) + 1);

	req->fragment = getFragment(url);
	req->queryString = getQueryString(url);
	req->path = getPath(url);
	req->filename = getFilename(req->path);

	free(url);
}

char *
getFragment(char *url)
{
	char *ptr;
	char *frag;

	if (url == NULL) {
		return NULL;
	}

	ptr = strchr(url, '#');
	if (ptr == NULL) {
		return NULL;
	}
	ptr++;

	if ((frag = malloc((strlen(ptr) + 1) * sizeof(char))) == NULL) {
		perror("malloc error");
		return NULL;
	}

	strncpy(frag, ptr, strlen(ptr) + 1);
	return frag;
}


char *
getQueryString(char *uri)
{
	char *startp, *endp;
	char *qs;

	if (uri == NULL) {
		return NULL;
	}

	startp = strchr(uri, '?');
	if (startp == NULL) {
		return NULL;
	}
	startp++;

	endp = strchr(startp, '#');

	// no fragment
	if (endp == NULL) {	
		if ((qs = malloc((strlen(startp) + 1) * sizeof(char))) == NULL) {
			perror("malloc error");
			return NULL;
		}
		strncpy(qs, startp, strlen(startp) + 1);
		return qs;
	} 

	// fragment
	if ((qs = malloc((endp - startp + 1) * sizeof(char))) == NULL) {
		perror("malloc error");
		return NULL;
	}

	strncpy(qs, startp, endp - startp);
	qs[endp-startp] = '\0';
	return qs;
}

char *
getPath(char *uri)
{
	char *path;
	char *startp, *endp;

	if (uri == NULL) {
		return NULL;
	}

	startp = uri;
	endp = strchr(startp, '?');

	// no queryString
	if (endp == NULL) {	
		// check for fragment
		endp = strchr(startp, '#');
		// no fragment
		if (endp == NULL) {
			if ((path = malloc((strlen(startp) + 1) * sizeof(char))) == NULL) {
				perror("malloc error");
				return NULL;
			}
			strncpy(path, startp, strlen(startp));
			path[strlen(startp)] = '\0';
			return path;
		}
	
		// fragment exists
		if ((path = malloc((endp - startp + 1) * sizeof(char))) == NULL) {
			perror("malloc error");
			return NULL;
		}
		strncpy(path, startp, endp - startp);
		path[endp-startp] = '\0';
		return path;
	} 

	// query string exists
	if ((path = malloc((endp - startp + 1) * sizeof(char))) == NULL) {
		perror("malloc error");
		return NULL;
	}

	strncpy(path, startp, endp - startp);
	path[endp-startp] = '\0';
	return path;
}

char *
getFilename(char *path)
{
	char *filename;
	char *startp, *endp;

	if (path == NULL) {
		return NULL;
	}

	startp = path;
	endp = path + strlen(path) - 1;

	while (startp < endp && *endp == '/') {
		endp--;
	}

	startp = strrchr(startp, '/');
	if (startp == NULL) {
		return NULL;
	}

	startp++;

	if (startp > endp) {	// path = "/"
		return NULL;
	}

	if ((filename = malloc((endp - startp + 2) * sizeof(char))) == NULL) {
		perror("malloc error");
		return NULL;
	}
	strncpy(filename, startp, endp - startp + 1);
	filename[endp - startp + 1] = '\0';

	return filename;
}


char *
decodeURL(char *uri)
{
	char *urip;
	char *buf;
	char *bufp;

	if (uri == NULL) {
		return NULL;
	}


	urip = uri;
	if ((buf = malloc((strlen(uri) + 1) * sizeof(char))) == NULL) {
		perror("malloc error");
		return uri;
	}  	

	bufp = buf;
	while (*urip != '\0') {
		if (*urip == '%') {
			if (urip[1] != '\0' && urip[2] != '\0') {
				*bufp = getOctetFromEscaped(urip[1], urip[2]);
				bufp++;
				urip += 2;
			}
		} else if (*urip == '+') {
//			*bufp = '\\';
//			bufp++;
			*bufp = ' ';
			bufp++;
		} else {
			*bufp = *urip;
			bufp++;
		}
		urip++;
	}
	*bufp = '\0';
	
	return buf; 
}

char 
getOctetFromEscaped(char hex1, char hex2)
{
	if (isxdigit((int)hex1) && isxdigit((int)hex2)) {
		return (char)((hexToDec(hex1) << 4) + hexToDec(hex2));
	}

	return 0;
}

char hexToDec(char ch)
{
	if (!isxdigit((int)ch)) {
		return 0;
	}
	
	if (isdigit((int)ch)) {
		return ch - '0';
	}
	
	return tolower((int)ch) - 'a' + 10;
}


/* 
 * check validity of url by checking validity of each part of it
 */
int 
isValidURL(Request *req)
{
	if (req == NULL) {
		return 0;
	}

	if (req->queryString != NULL) {
		if (!isValidQueryOrFrag(req->queryString)) {	
			return 0;
		}
	}

	if (req->fragment != NULL) {
		if (!isValidQueryOrFrag(req->fragment)) {	
			return 0;
		}
	}

	if (req->path != NULL) {
		if (!isValidPath(req->path)) {
			return 0;
		}
	}

	return 1;
}

/*
 * query 	= *(pchar | '/' | '?')
 * fragment = *(pchar | '/' | '?')
 * Since these two have same rules, we use one function for validity check of both.
 * returns 1 if valid, otherwise returns 0
 */
int
isValidQueryOrFrag(char *qs)
{
	int len;

	if (qs == NULL) {
		return 0;
	}

	if (strcmp(qs, "") == 0) {
		return 1;
	}

	while (*qs) {
		if (*qs == '/' || *qs == '?') {
			qs++;
		} else { 
			len = isPchar(qs);
			if (len == 0) {
				return 0;
			} else {
				qs += len;
			}
		}
	}

	return 1;
}

/* in rfc: path = path-abempty | path-absolute | path-noscheme | path-rootless | path-empty
 * This function implements one rule: 
 * path = path-absolute ; begins with "/" but not  "//"
 */
int
isValidPath(char *path)
{
	if (path == NULL) {
		printf("a");
		return 0;
	}

	return isPathAbsolute(path);
}

/*
 * path-absolute = "/" [segment * ("/" segment)] 
 */
int 
isPathAbsolute(char *path)
{
	char *startp, *endp;

	startp = path;
	if (*startp != '/') {
		printf("b");
		return 0;
	}

	startp++;
	while((endp = strchr(startp, '/')) != NULL) {
		if (!isSegment(startp, endp -1)) {
			printf("c");
			return 0;
		}
		startp = endp + 1;
	}

	if (startp < path + strlen(path)) {
		return isSegment(startp, path + strlen(path) - 1);
	}

//	printf("d");
	return 1;
}

/*
 * segment = *pchar in rfc, meaning that segment can be empty
 * In this function: segment = 1*pchar
 * meaning that segment must be non-empty 
 */
int 
isSegment(char *startp, char *endp)
{
	if (startp > endp) {
		printf("d");
		return 0;
	}

	while (*startp && *endp && startp <= endp) {
		if (isPchar(startp)) {
			startp++;
		} else {
			printf("e");
			return 0;
		}
	}
	return 1;
}

/*
 * pchar = unreserved | pct-encoded | sub-delims | ':' | '@'
 * if the pointed str points to a char which is not pchar, it returns 0
 * it the pointer str points to a pct-encoded char triplet, it returns 3
 * if the pointer str poinst to a single valid pchar char, it returns 1
 */
int 
isPchar(char *str)
{
	if (str == NULL) {
		return 0;
	}

	if (isUnreserved(*str) || isSubDelims(*str) || (*str == ':') || (*str == '@')) {
		return 1;
	} else if (str[1] && str[2]) {
		if (isPctEncoded(str[0], str[1], str[2])) {
			return 3;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

/*
 * unreserved = ALPHA | DIGIT | '-' | '.' | '_' | '~'
 */
int
isUnreserved(char ch)
{
	if (isalnum((int)ch)) {
		return 1;
	}

	if (ch == '-' || ch == '.' || ch == '_' || ch == '~') {
		return 1;
	}

	return 0;
}

/*
 * sub-delims = "!" | "$" | "&" | "'" | "(" | ")" | "*" | "+" | "," | ";" | "="
 */
int 
isSubDelims(char ch)
{
	if (ch == '!' || ch == '$' || ch == '&' || ch == '\'' || ch == '(' ||
	ch == ')' || ch == '*' || ch == '+' || ch == ',' || ch == ';' || ch == '=') { 
		return 1;
	}

	return 0;
}

/*
 * pct-encoded = '%' hexdigit hexdigit
 */
int
isPctEncoded(char ch, char hex1, char hex2)
{
	if (ch == '%' && isxdigit((int)hex1) && isxdigit((int)hex2)) {
		return 1;
	}

	return 0;
}

/*
 * reserved = gen-delims | sub-delims
 */
int 
isReserved(char ch)
{
	if (isGenDelims(ch) || isSubDelims(ch)) { 
		return 1;
	}

	return 0;
}

/*
 * gen-delims = ":" | "/" | "?" | "#" | "[" | "]" | "@"
 */
int 
isGenDelims(char ch)
{
	if (ch == ':' || ch == '/' || ch == '?' || ch == '#' || ch == '[' ||
	ch == ']' || ch == '@') { 
		return 1;
	}

	return 0;
}


