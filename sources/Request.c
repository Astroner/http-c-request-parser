#include "Request.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

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

typedef enum ParsingStage {
    ParsingStageMethod,
    ParsingStagePath,
    ParsingStageParams,
    ParsingStageHeaders,
} ParsingStage;

#define METHOD_SIZE_LIMIT 6

static int parseMethod(Request* req, size_t* endIndex) {
    int waitForSpace = 0;

    if(req->dataLength < METHOD_SIZE_LIMIT) return -1;

    for(int i = 0; i < METHOD_SIZE_LIMIT; i++) {
        char ch = req->buffer[i];
        if(waitForSpace) {
            if(ch == CHAR_SPACE) {
                *endIndex = i - 1;
                return 0;
            }
        } else if(i == 0) {
            if(ch == CHAR_G) {
                req->method = RequestMethodGet;
                waitForSpace = 1;
            } else if(ch == CHAR_D) {
                req->method = RequestMethodDelete;
                waitForSpace = 1;
            } else if(ch == CHAR_P) {
                continue;
            } else {
                return -1;
            }
        } else if(i == 1) {
            if(ch == CHAR_O) {
                req->method = RequestMethodPost;
                waitForSpace = 1;
            } else if(ch == CHAR_U) {
                req->method = RequestMethodPut;
                waitForSpace = 1;
            } else if(ch == CHAR_A) {
                req->method = RequestMethodPatch;
                waitForSpace = 1;
            } else {
                return -1;
            }
        } else {
            return -1;
        }
    }

    return -1;
}

static int parseParams(size_t start, Request* req, size_t* endIndex) {
    size_t keyStart = start;
    size_t keyEnd = 0;
    size_t valueStart = 0;
    for(size_t i = start; i < req->dataLength; i++) {
        char ch = req->buffer[i];
        if(ch == CHAR_EQUAL) {
            keyEnd = i - 1;
            valueStart = i + 1;
        } else if(ch == CHAR_AMP) {
            if(keyStart == i) return -1;
            if(!valueStart) {
                HashTable_set(req->params, keyStart, i - 1, keyStart, i - 1);
            } else {
                HashTable_set(req->params, keyStart, keyEnd, valueStart, i - 1);
            }
            keyStart = i + 1;
            keyEnd = 0;
            valueStart = 0;
        } else if(ch == CHAR_SPACE) {
            if(i == start) {
                *endIndex = i - 1;
                return 0;
            }
            if(keyStart == i) return -1;
            *endIndex = i - 1;

            if(!valueStart) {
                HashTable_set(req->params, keyStart, i - 1, keyStart, i - 1);
            } else {
                HashTable_set(req->params, keyStart, keyEnd, valueStart, i - 1);
            }
            return 0;
        }
    }

    return -1;
}

static int parsePath(size_t start, Request* req, size_t* endIndex) {
    if(req->buffer[start] != CHAR_SLASH) return -1;

    req->originalUrl.start = start;
    req->path.start = start;
    
    for(size_t i = start; i < req->dataLength; i++) {
        char ch = req->buffer[i];
        if(ch == CHAR_QUESTION_MARK) {
            req->path.end = i - 1;

            if(parseParams(i + 1, req, endIndex) < 0) {
                return -2;
            }

            req->originalUrl.end = *endIndex;
            return 0;
        } else if(ch == CHAR_SPACE) {
            req->path.end = i - 1;
            req->originalUrl.end = i - 1;
            
            *endIndex = i - 1;
            return 0;
        }
    }

    return -1;
}

static int parseHeaders(size_t start, Request* req, size_t* endIndex) {
    size_t nameStart = start;
    size_t nameEnd = 0;
    size_t valueStart = 0;

    for(size_t i = start; i < req->dataLength; i++) {
        char ch = req->buffer[i];
        if(ch == CHAR_COLON && !nameEnd) {
            nameEnd = i - 1;
            valueStart = i + 2;
        } else if(ch == CHAR_CR) {
            if(req->buffer[i + 1] != CHAR_NL) return -1;
            i++;
            if(valueStart) {
                HashTable_set(req->headers, nameStart, nameEnd, valueStart, i - 2);
                nameStart = i + 1;
                nameEnd = 0;
                valueStart = 0;
            } else if(nameStart == i - 1) {
                *endIndex = i;
                return 0;
            }
        }
    }

    return -2;
}

static int parseBuffer(Request* req) {
    size_t lastIndex = 0;

    int exitCode = 0;

    if((exitCode = parseMethod(req, &lastIndex)) < 0) {
        printf("Failed to parse method (%d)\n", exitCode);
        return -1;
    };
    if((exitCode = parsePath(lastIndex + 2, req, &lastIndex)) < 0) {
        printf("Failed to parse path (%d)\n", exitCode);
        return -2;
    }
    if(strncmp("HTTP/1.1\r\n", req->buffer + lastIndex + 2, 10) != 0) {
        printf("Failed to find \"HTTP/1.1\" pattern\n");
        return -3;
    }

    lastIndex += 2 + 9;

    if((exitCode = parseHeaders(lastIndex + 1, req, &lastIndex)) < 0) {
        printf("Failed to parse headers (%d)\n", exitCode);
        return -5;
    }

    req->payloadStartIndex = lastIndex + 1;

    return 0;
}

int Request_parseSocket(int socket, Request* result) {
    size_t dataLength = read(socket, result->buffer, result->bufferLength);

    result->dataLength = dataLength;

    return parseBuffer(result);
}

char* Request_getPath(Request* req) {
    return extractRange(req->buffer, req->path.start, req->path.end);
}

char* Request_getHeader(Request* request, char* name) {
    Range* range = HashTable_get(request->headers, name);

    if(!range) return NULL;

    return extractRange(request->buffer, range->start, range->end);
};

char* Request_getPayload(Request* request) {
    return request->buffer + request->payloadStartIndex;
}

void Request_reset(Request* request) {
    memset(request->buffer, '\0', request->bufferLength);
    HashTable_clear(request->headers);
    HashTable_clear(request->params);

    request->dataLength = 0;

    request->originalUrl.start = 0;
    request->originalUrl.end = 0;

    request->path.start = 0;
    request->path.end = 0;

    request->payloadStartIndex = 0;
}

void Request_print(Request* request) {
    char* methodLabel = 
        request->method == RequestMethodGet
        ? "GET"
        : request->method == RequestMethodPost
        ? "POST"
        : request->method == RequestMethodPut
        ? "PUT"
        : request->method == RequestMethodPatch
        ? "PATCH"
        : request->method == RequestMethodDelete
        ? "DELETE"
        : "UNKNOWN";

    printf("## Method: %s\n", methodLabel);
    printf("## Original URL: ");
    printRange(request->buffer, request->originalUrl.start, request->originalUrl.end, PrintRangeModeDefault);
    printf("\n");

    printf("## Path: ");
    printRange(request->buffer, request->path.start, request->path.end, PrintRangeModeDefault);
    printf("\n");

    Iterator iterator;
    HashTableItem* item;

    if(request->params->values > 0) {
        printf("## Params: %zu\n", request->params->values);
        HashTable_initIterator(request->params, &iterator);
        while((item = Iterator_next(&iterator))) {
            printf("### ");
            printRange(request->buffer, item->key.start, item->key.end, PrintRangeModeDefault);
            printf(": ");
            printRange(request->buffer, item->value.start, item->value.end, PrintRangeModeDefault);
            printf("\n");
        }
    }

    printf("## Headers: %zu\n", request->headers->values);
    HashTable_initIterator(request->headers, &iterator);
    while((item = Iterator_next(&iterator))) {
        printf("### ");
        printRange(request->buffer, item->key.start, item->key.end, PrintRangeModeDefault);
        printf(": ");
        printRange(request->buffer, item->value.start, item->value.end, PrintRangeModeDefault);
        printf("\n");
    }

    if(request->payloadStartIndex < request->dataLength) {
        printf("## Payload: (%zu)\n", request->dataLength - request->payloadStartIndex);
        printf("%s\n", request->buffer + request->payloadStartIndex);
    } else {
        printf("## No Payload\n");
    }
}
