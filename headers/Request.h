#if !defined(REQUEST_H)
#define REQUEST_H

#include <stdlib.h>

#include "main.h"
#include "HashTable.h"

#define CHAR_SPACE 32
#define CHAR_NL 10
#define CHAR_G 71
#define CHAR_D 68
#define CHAR_P 80
#define CHAR_O 79
#define CHAR_U 85
#define CHAR_A 65
#define CHAR_COLON 58
#define CHAR_CR 13 /* \r */
#define CHAR_QUESTION_MARK 63
#define CHAR_EQUAL 61
#define CHAR_AMP 38 /* & */
#define CHAR_SLASH 47 /* / */

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
    memset(name##__buffer, '\0', bufferSize);\
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
        .valueBufferSize = headerNumber,\
    };\
\
    HashTable name##__params = {\
        .keysBuffer = name##__buffer,\
        .keysBufferLength = sizeof(name##__buffer),\
\
        .valueBuffer = name##__paramsBuffer,\
        .valueBufferSize = paramsNumber,\
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

Request* Request_create(size_t bufferSize, size_t headersNumber, size_t paramsNumber);
void Request_reset(Request* request);
void Request_free(Request* request);

char* Request_getPath(Request* request);
char* Request_getHeader(Request* request, char* name);
char* Request_getPayload(Request* request);
void Request_print(Request* request);
int Request_hasPayload(Request* request);
size_t Request_payloadLength(Request* request);

#endif // REQUEST_H
