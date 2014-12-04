#include <stdio.h>
#include "error.h"
#include "response.h"

void 
handleError(int code, int client_fd)
{
    response_status(code, client_fd);
    response_header(NULL, 0, client_fd);
    response_message(-1, client_fd);
}

