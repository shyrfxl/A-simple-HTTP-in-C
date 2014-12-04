
#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>

#include "daemon.h"

void daemonlize () {
    pid_t pid;
    char *dir = "/";
    /* create daemon process */
    pid = fork();
    
    if (pid < 0) {
        fprintf(stderr, "Unable to create a process: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pid > 0) 
        exit(EXIT_SUCCESS);

    umask(0);

    if (setsid() == -1) {
        fprintf(stderr, "Unable to create session and set process group ID: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (chdir(dir) < 0) {
        fprintf(stderr, "Unable to change to directory '%s': %s\n", dir, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}
