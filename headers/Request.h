#if !defined(REQUEST_H)
#define REQUEST_H

#include <stdlib.h>

#include "main.h"
#include "HashTable.h"

typedef enum RequestMethod {
    RequestMethodGet,
    RequestMethodPost,
    RequestMethodPut,
    RequestMethodPatch,
    RequestMethodDelete,
} RequestMethod;

typedef struct Request {
    Range path;
    Range originalUrl;
    RequestMethod method;


    HashTable* headers;
    HashTable* params;

    size_t payloadStartIndex;

    char* buffer;
    size_t bufferSize;
    size_t dataLength;
} Request;

int Request_parseSocket(int socket, Request* result);

char* Request_getPath(Request* request);
char* Request_getHeader(Request* request, char* name);
char* Request_getPayload(Request* request);
void Request_reset(Request* request);

#endif // REQUEST_H
