/*
 *  HTTP.c   
 *    handle the http part of the server
 *    resolve the path, get the content 
 *    and construct the response
 *
 *
 */

#include <sys/types.h>
#include <sys/stat.h>

#if defined(__linux__) ||defined(__NetBSD__)
#include <magic.h>
#include <fts.h>
#endif
#if defined(__sun__)
#include <ast/ast.h>
#include <ast/magic.h>
#include <ast/fts.h>
#endif

#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <pwd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "http.h"
#include "request.h"
#include "response.h"
#include "logging.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef MESSAGE_MAX
#define MESSAGE_MAX 10240
#endif

#ifndef FTS_ROOTLEVEL
#define FTS_ROOTLEVEL 0
#endif

int
#if defined(__linux__) || defined(__NetBSD__)
namecmp(const FTSENT **a, const FTSENT **b)
#endif
#if defined(__sun__)
namecmp(FTSENT * const *a, FTSENT * const *b)
#endif
{
    if((*a)->fts_info == FTS_ERR || (*b)->fts_info == FTS_ERR)
        return 0;
    return (strcmp((*a)->fts_name, (*b)->fts_name));
}


int generateDirIndex(char *path, char *realPath, char *message, int len) {
    FTS *fts;
    FTSENT *p;
    char *pathArray[2];
    pathArray[0] = realPath;
    pathArray[1] = NULL;
    if ((fts = fts_open(pathArray, FTS_PHYSICAL, &namecmp)) == NULL) {
        return 400;
    }
    if(len < strlen(path) * 2 + strlen("<html><head><title>Index of </title></head><body><h1>Index of </h1><ul>"))
        return 500;
    sprintf(message, "<html><head><title>Index of %s</title></head><body><h1>Index of %s</h1><ul>", path, path);
    while((p = fts_read(fts))) {
        switch(p->fts_info) {
            case FTS_DC:
            case FTS_DNR:
            case FTS_ERR:
                break;
            case FTS_DP:
            case FTS_DOT:
                break;
            case FTS_D:
                if(p->fts_level == FTS_ROOTLEVEL)
                    break;
                fts_set(fts, p, FTS_SKIP);
                if(p->fts_name[0] == '.')
                    break;
                if(strlen(message) + 2 * p->fts_namelen + strlen("<li><a href=\"/\">/</a></li>") > len)
                    return 500;
                sprintf(message+strlen(message), "<li><a href=\"%s/\">%s/</a></li>", p->fts_name, p->fts_name);
                break;
            case FTS_F:
            case FTS_SL:
            default:
                if(p->fts_name[0] == '.')
                    break;
                if(strlen(message) + 2 * p->fts_namelen + strlen("<li><a href=\"\"></a></li>") > len)
                    return 500;
                sprintf(message+strlen(message), "<li><a href=\"%s\">%s</a></li>", p->fts_name, p->fts_name);
                break;
        }
    }
    fts_close(fts);
    if(strlen(message) + strlen("</ul></body></html>") > len)
       return 500; 
    sprintf(message+strlen(message), "</ul></body></html>");
    return 0;
}



#if defined(__linux__) || defined(__NetBSD__)
void
magicType (const char *resource, char *types) {
    magic_t magic;
    const char *mime;
    magic = magic_open(MAGIC_MIME_TYPE);
    magic_load(magic, NULL);
    magic_compile(magic, NULL);
    mime = magic_file(magic, resource);
    strcpy(types, mime);
    magic_close(magic);
}
#endif

#if defined(__sun__)
void 
magicType(const char *resource, char *types){
    strcpy(types, "text/plain");
}
#endif

#ifdef strftime
#undef strftime
#endif
void 
handleHTTP(Request *req, char *HTTProot, int socket_fd)
{
    char path[PATH_MAX];
    int status;
    struct stat fileStat;
    int file_fd;
    char header[3][2][HEADER_MAX];

    if((req->path)[0] != '/'){
        handleError(400, socket_fd);
        swsLog(req, 400, 0);
        return;
    }
    status = resolvePath(HTTProot, req->path, path);
    if(status != 0){
        handleError(status, socket_fd);
        swsLog(req, status, 0);
        return;
    }
    //printf("ResolvedPath: %s\n", path);

    //seperate file and dir
    status = 0;
    status = stat(path, &fileStat);
    if(status != 0){
        handleError(403, socket_fd);
        swsLog(req, 403, 0);
        return;
    }
    if(S_ISREG(fileStat.st_mode)){
        char str_date[200];
        char mimeType[HEADER_MAX];
        
        if(req->t != 0 && fileStat.st_mtime - req->t <= 0){
            handleError(304, socket_fd);
            swsLog(req, 304, 0);
            return;
        }
        if((file_fd = open(path, O_RDONLY)) < 0){
            handleError(404, socket_fd);
            swsLog(req, 404, 0);
            return;
        }
        sprintf(header[0][0], "Last-Modified");
        strftime(str_date, 200, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&fileStat.st_mtime));
        str_date[strlen(str_date)] = '\0';
        sprintf(header[0][1], "%s", str_date);
        sprintf(header[1][0], "Content-Type");
        magicType(path, mimeType);
        sprintf(header[1][1], "%s", mimeType);
        sprintf(header[2][0], "Content-Length");
        sprintf(header[2][1], "%d", (int)fileStat.st_size);

        response_status(200, socket_fd);
        response_header(header, 3, socket_fd);
        if(strcmp(req->method, "HEAD") == 0){
            response_message(-1, socket_fd);
            swsLog(req, 200, 0);
        }else{
            response_message(file_fd, socket_fd);
            swsLog(req, 200, (unsigned)fileStat.st_size);
        }
        close(file_fd);
        return;
    }else if (S_ISDIR(fileStat.st_mode)) {
        char indexFile[PATH_MAX];
        sprintf(indexFile, "%s/index.html", path);
        status = 0;
        status = stat(indexFile, &fileStat);
        if(status ==0){
            if(req->t != 0 && fileStat.st_mtime - req->t <= 0){
                handleError(304, socket_fd);
                swsLog(req, 304, 0);
                return;
            }
            if((file_fd = open(indexFile, O_RDONLY)) >= 0){
                char str_date[200];

                sprintf(header[0][0], "Last-Modified");
                strftime(str_date, 200, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&fileStat.st_mtime));
                str_date[strlen(str_date)] = '\0';
                sprintf(header[0][1], "%s", str_date);
                sprintf(header[1][0], "Content-Type");
                sprintf(header[1][1], "%s", "text/html");
                sprintf(header[2][0], "Content-Length");
                sprintf(header[2][1], "%d", (int)fileStat.st_size);

                response_status(200, socket_fd);
                response_header(header, 3, socket_fd);
                if(strcmp(req->method, "HEAD") == 0){
                    response_message(-1, socket_fd);
                    swsLog(req, 200, 0);
                }else{
                    response_message(file_fd, socket_fd);
                    swsLog(req, 200, (unsigned)fileStat.st_size);
                }
                close(file_fd);
                return;
            }
            handleError(500, socket_fd);
            swsLog(req, 500, 0);
        }else{
            char message[MESSAGE_MAX];
            
            status = 0;
            status = generateDirIndex(req->path, path, message, MESSAGE_MAX);
            if(status != 0){
                handleError(status, socket_fd);
                swsLog(req, status, 0);
                return;
            }

            sprintf(header[0][0], "Content-Type");
            sprintf(header[0][1], "%s", "text/html");
            sprintf(header[1][0], "Content-Length");
            sprintf(header[1][1], "%d", (int)strlen(message));

            response_status(200, socket_fd);
            response_header(header, 2, socket_fd);
            if(strcmp(req->method, "HEAD") == 0) {
                response_message(-1, socket_fd);
                swsLog(req, 200, 0);
            }else{
                write(socket_fd, message, strlen(message));
                response_message(-1, socket_fd);
                swsLog(req, 200, strlen(message));
            }
            return;
        }
    }else{
        handleError(400, socket_fd);
        swsLog(req, 400, 0);
        return;
    }
}


int
resolvePath(char *HTTPRoot, char *uri, char *resolvedPath)
{
    if(uri[1] == '~'){
        // User Directories root
        char *urip, *nameEnd;
        char username[PATH_MAX];
        char userDirRoot[PATH_MAX];
        char path[PATH_MAX];
        struct passwd *userpasswd;

        nameEnd = NULL;
        urip = uri + 2;
        nameEnd = strchr(urip, '/');
        if(nameEnd == NULL){
            strncpy(username, urip, strlen(urip));
        }else{
            strncpy(username, urip, nameEnd-urip);
        }
        userpasswd = getpwnam(username);
        if(userpasswd == NULL)
            return 403;
        sprintf(userDirRoot, "%s/sws", userpasswd->pw_dir);
        if(nameEnd != NULL)
            sprintf(path, "%s/%s", userDirRoot, nameEnd + 1);
        else
            sprintf(path, "%s", userDirRoot);
        realpath(path, resolvedPath);
        if(resolvedPath == NULL)
            return 500;
        if(validatePath(resolvedPath, userDirRoot) == 0)
            return 0;
        else
            return 403;
    }else{
        // HTTP dir root
        char path[PATH_MAX];
        char realRootPath[PATH_MAX];

        realpath(HTTPRoot, realRootPath);
        sprintf(path, "%s/%s", realRootPath, uri);
        realpath(path, resolvedPath);
        if(resolvedPath == NULL){
            return 500;
        }
        if(validatePath(resolvedPath, realRootPath) == 0){
            return 0;
        }else
            return 403;
    }
}


int 
validatePath(char *path, char *HTTPRoot)
{
    if(strstr(path, HTTPRoot) != path)
        return -1;
    return 0;
}
