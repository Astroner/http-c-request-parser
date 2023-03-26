#include "Request.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#define METHOD_SIZE_LIMIT 6

static int parseMethod(Request* req, size_t* endIndex) {
    int waitForSpace = 0;

    if(req->dataLength < METHOD_SIZE_LIMIT) return -1;

    for(int i = 0; i < METHOD_SIZE_LIMIT; i++) {
        char ch = req->buffer[i];

        // The 'If' order is not really optimal, it would be better to place waitForSpace case first, but it was made for simplicity

        // So here we will try to identify the method by 2 characters
        if(i == 0) {
            switch(ch) {
                case CHAR_G: // If the first character is "G" then it is obviously GET a request and so on
                    req->method = RequestMethodGet;
                    waitForSpace = 1;
                    break;
                case CHAR_D:
                    req->method = RequestMethodDelete;
                    waitForSpace = 1;
                    break;
                case CHAR_P: // In "P" case we have to read second character
                    continue;
                default:
                    return -1;
            }
        } else if(i == 1) {
            switch(ch) {
                case CHAR_O:
                    req->method = RequestMethodPost;
                    waitForSpace = 1;
                    break;
                case CHAR_U:
                    req->method = RequestMethodPut;
                    waitForSpace = 1;
                    break;
                case CHAR_A:
                    req->method = RequestMethodPatch;
                    waitForSpace = 1;
                    break;
                default:
                    return -2;
            }
        } else if(waitForSpace) { // If we already have the method then we just wait for Space.
            if(ch == CHAR_SPACE) {
                *endIndex = i - 1;
                return 0;
            }
        } else {
            return -3;
        }
    }

    return -4;
}

static int parseParams(size_t start, Request* req, size_t* endIndex) {
    // Parameter string consist of key-value pairs in specific format: "key1=value1&key2=value2&key3&key4=value4"
    // so we have 2 important chars: 
    // "=" that indicates the end of the param name and the beginning of the value 
    // and "&" that indicates the end of the value and the beginning the next name
    size_t keyStart = start;
    size_t keyEnd = 0;
    size_t valueStart = 0;
    for(size_t i = start; i < req->dataLength; i++) {
        char ch = req->buffer[i];
        if(ch == CHAR_EQUAL) {
            keyEnd = i - 1;
            valueStart = i + 1;
        } else if(ch == CHAR_AMP) {
            if(keyStart == i) return -1;// format error: /?&=222 || /?name=value&&
            if(!valueStart) { // "/?booleanKey" scenario
                HashTable_set(req->params, keyStart, i - 1, keyStart, i - 1);
            } else {
                HashTable_set(req->params, keyStart, keyEnd, valueStart, i - 1);
            }
            keyStart = i + 1;
            keyEnd = 0;
            valueStart = 0;
        } else if(ch == CHAR_SPACE) { // the end of the path
            if(i == start) { // "/? " scenario
                *endIndex = i - 1;
                return 0;
            }
            if(keyStart == i) {
                // format error: "/?name=value& "
                // because keyStart can point to space char only in scenario in which we get "&" except "/?" scenario which is handled above
                return -1;
            }
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
    // just to ensure that everything is in order
    if(req->buffer[start] != CHAR_SLASH) return -1;

    // Setting up ranges start
    req->originalUrl.start = start;
    req->path.start = start;
    
    // Then we just iterating over the buffer looking for question mark(?) for params and space as the end of the path
    for(size_t i = start; i < req->dataLength; i++) {
        char ch = req->buffer[i];
        if(ch == CHAR_QUESTION_MARK) {// means that we have parameters in the url
            req->path.end = i - 1; // setting up path end

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
    // headers parsing is simillar to params parsing except key-value separator and entry separator
    // ":" divides key and value
    // "\r\n" divides entries
    size_t nameStart = start;
    size_t nameEnd = 0;
    size_t valueStart = 0;

    for(size_t i = start; i < req->dataLength; i++) {
        char ch = req->buffer[i];
        // ":"(colon) can appear inside of the value, so we need to check for it using nameEnd
        if(ch == CHAR_COLON && !nameEnd) {
            nameEnd = i - 1;
            valueStart = i + 2;
        } else if(ch == CHAR_CR) { // "\r\n" check
            // if the next char is not "\n" then its format error
            if(req->buffer[i + 1] != CHAR_NL) return -1;
            i++; // step over "\n"
            if(valueStart) { // if we have value start then just set new header
                HashTable_set(req->headers, nameStart, nameEnd, valueStart, i - 2);
                nameStart = i + 1;
                nameEnd = 0;
                valueStart = 0;
            } else if(nameStart == i - 1) {// else its a second "\r\n" in a row which points the end of headers
                *endIndex = i;
                return 0;
            }
        } else if(ch == CHAR_NL) { // single "\n"s are not allowed 
            return -2;
        }
    }

    return -3;
}

static int parseBuffer(Request* req) {
    // So, here we need to read the buffer step-by-step
    // Method => Path(Params) => Headers => rest is the payload
    
    // This variable indicates current cursor position and will be moved by all the parer functions
    size_t cursor = 0;

    int exitCode = 0;

    if((exitCode = parseMethod(req, &cursor)) < 0) {
        printf("Failed to parse method (%d)\n", exitCode);
        return -1;
    };
    if((exitCode = parsePath(cursor + 2, req, &cursor)) < 0) {
        printf("Failed to parse path (%d)\n", exitCode);
        return -2;
    }
    if(strncmp("HTTP/1.1\r\n", req->buffer + cursor + 2, 10) != 0) {
        printf("Failed to find \"HTTP/1.1\" pattern\n");
        return -3;
    }

    cursor += 2 + 9;

    if((exitCode = parseHeaders(cursor + 1, req, &cursor)) < 0) {
        printf("Failed to parse headers (%d)\n", exitCode);
        return -5;
    }

    req->payloadStartIndex = cursor + 1;

    return 0;
}

int Request_parseSocket(int socket, Request* result) {
    // first, we need to read data to the buffer
    // it assumes that Request object already was initialized and allocated
    size_t dataLength = recv(socket, result->buffer, result->bufferLength, 0);

    // read() function returns the length of the content read
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

int Request_hasPayload(Request* request) {
    return (request->dataLength - request->payloadStartIndex) > 0;
}

size_t Request_payloadLength(Request* request) {
    return request->dataLength - request->payloadStartIndex;
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

Request* Request_create(size_t bufferSize, size_t headersNumber, size_t paramsNumber) {
    size_t memorySize = 
        sizeof(Request)
        + bufferSize 
        + sizeof(HashTable)
        + headersNumber * sizeof(HashTableItem) 
        + sizeof(HashTable)
        + paramsNumber * sizeof(HashTableItem);

    char* memory = malloc(memorySize);

    if(!memory) return NULL;

    memset(memory, '\0', memorySize);

    Request* req = (void*)memory;
    char* buffer = (void*)req + sizeof(Request);
    HashTable* headers = (void*)buffer + bufferSize;
    HashTableItem* headersBuffer = (void*)headers + sizeof(HashTable);
    HashTable* params = (void*)headersBuffer + headersNumber * sizeof(HashTableItem);
    HashTableItem* paramsBuffer = (void*)params + sizeof(HashTable);


    headers->keysBuffer = buffer;
    headers->keysBufferLength = bufferSize;
    headers->valueBuffer = headersBuffer;
    headers->valueBufferSize = headersNumber;



    params->keysBuffer = buffer;
    params->keysBufferLength = bufferSize;
    params->valueBuffer = paramsBuffer;
    params->valueBufferSize = paramsNumber;



    req->buffer = buffer;
    req->bufferLength = bufferSize;
    req->headers = headers;
    req->params = params;


    return req;
}

void Request_free(Request* request) {
    free(request);
}