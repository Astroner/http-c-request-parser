#if !defined(REQUEST_H)
#define REQUEST_H

#include <stdlib.h>

#include "main.h"

typedef enum RequestMethod {
    RequestMethodGet,
    RequestMethodPost,
    RequestMethodPut,
    RequestMethodPatch,
    RequestMethodDelete,
} RequestMethod;

typedef struct Request {
    Range path;
    RequestMethod method;

    char* buffer;
    size_t bufferSize;
    size_t contentSize;
} Request;

int Request_parseSocket(int socket, char* readBuffer, size_t readBufferLength, Request* result);

char* Request_getPath(Request* request);
char* Request_getHeader(Request* request, char* name);

#endif // REQUEST_H
