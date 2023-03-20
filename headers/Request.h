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
    size_t bufferLength;
    size_t dataLength;
} Request;

#define createStaticRequest(name, bufferSize, headerNumber, paramsNumber)\
    char name##__buffer[bufferSize];\
    memset(name##__buffer, '\0', sizeof(name##__buffer));\
\
    HashTableItem name##__headersBuffer[headerNumber];\
    memset(name##__headersBuffer, 0, sizeof(name##__headersBuffer));\
\
    HashTableItem name##__paramsBuffer[paramsNumber];\
    memset(name##__paramsBuffer, 0, sizeof(name##__paramsBuffer));\
\
    HashTable name##__headers = {\
        .keysBuffer = name##__buffer,\
        .keysBufferLength = sizeof(name##__buffer),\
\
        .valueBuffer = name##__headersBuffer,\
        .valueBufferSize = sizeof(name##__headersBuffer) / sizeof(HashTableItem),\
    };\
\
    HashTable name##__params = {\
        .keysBuffer = name##__buffer,\
        .keysBufferLength = sizeof(name##__buffer),\
\
        .valueBuffer = name##__paramsBuffer,\
        .valueBufferSize = sizeof(name##__paramsBuffer) / sizeof(HashTableItem),\
    };\
\
    Request name##__data = {\
        .buffer = name##__buffer,\
        .bufferLength = sizeof(name##__buffer),\
        .headers = &name##__headers,\
        .params = &name##__params,\
    };\
\
    Request* name = &name##__data;

int Request_parseSocket(int socket, Request* result);

char* Request_getPath(Request* request);
char* Request_getHeader(Request* request, char* name);
char* Request_getPayload(Request* request);
void Request_reset(Request* request);
void Request_print(Request* request);

#endif // REQUEST_H
