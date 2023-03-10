#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "HashTable.h"
#include "Request.h"

char* extractRange(char* buffer, size_t start, size_t end) {
    char* str = malloc(end - start + 2);

    memcpy(str, &buffer[start], end - start + 1);

    str[end + 1] = '\0';

    return str;
}

void printRange(char* buffer, size_t start, size_t end) {
    for(size_t i = start; i <= end; i++) {
        printf("%c", buffer[i]);
    }
}

int main(void) {
    int mainSocket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr = {
            .s_addr = htonl(INADDR_ANY),
        },
        .sin_port = htons(2020),
    };

    if(bind(mainSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) return 1;

    if(listen(mainSocket, 3) < 0) return 2;

    int incomingSocket;
    struct sockaddr_in incomingAddress;
    socklen_t incomingAddressLength;

    char requestBuffer[2048];
    HashTableItem headersBuffer[100];
    HashTableItem paramsBuffer[100];
    memset(paramsBuffer, 0, sizeof(paramsBuffer));
    memset(requestBuffer, 0, sizeof(requestBuffer));
    memset(headersBuffer, 0, sizeof(headersBuffer));
    HashTable headers = {
        .keysBuffer = requestBuffer,
        .keysBufferLength = sizeof(requestBuffer),

        .valueBuffer = headersBuffer,
        .valueBufferSize = sizeof(headersBuffer) / sizeof(HashTableItem),
    };

    HashTable params = {
        .keysBuffer = requestBuffer,
        .keysBufferLength = sizeof(requestBuffer),

        .valueBuffer = paramsBuffer,
        .valueBufferSize = sizeof(paramsBuffer) / sizeof(HashTableItem),
    };

    Request request = {
        .buffer = requestBuffer,
        .bufferSize = sizeof(requestBuffer),
        .headers = &headers,
        .params = &params,
    };

    while(1) {
        if((incomingSocket = accept(mainSocket, (struct sockaddr*)&incomingAddress, &incomingAddressLength)) < 0) return 3;

        printf("# New Request\n");
        int status;
        if((status = Request_parseSocket(incomingSocket, &request)) < 0) {
            printf("Failed to parse request: %d\n", status);
        };

        char* contentLength;
        if(!(contentLength = Request_getHeader(&request, "Content-Length"))) {
            write(incomingSocket, "HTTP/1.1 400 Bad Request\nContent-Type: text/plain\nContent-Length: 26\n\nNo Content-Length provided", 96);
            close(incomingSocket);
            continue;
        }

        size_t contentLengthParsed = atol(contentLength);
        size_t payloadLength = request.dataLength - request.payloadStartIndex;

        if(contentLengthParsed != payloadLength) {
            write(incomingSocket, "HTTP/1.1 400 Bad Request\nContent-Type: text/plain\nContent-Length: 20\n\nWrong Content Length", 90);
            close(incomingSocket);
            continue;
        }

        char* methodString = 
            request.method == RequestMethodGet
            ? "GET"
            : request.method == RequestMethodPost
            ? "POST"
            : request.method == RequestMethodPut
            ? "PUT"
            : request.method == RequestMethodPatch
            ? "PATCH"
            : "DELETE";

        printf("## Original URL: ");
        printRange(request.buffer, request.originalUrl.start, request.originalUrl.end);
        printf("\n");
        printf("## Method: %s\n", methodString);
        printf("## Headers: %zu\n", request.headers->values);

        Iterator iterator;
        HashTable_initIterator(&headers, &iterator);
        HashTableItem* item;
        while((item = Iterator_next(&iterator))) {
            printf("### ");
            printRange(request.buffer, item->key.start, item->key.end);
            printf(": ");
            printRange(request.buffer, item->value.start, item->value.end);
            printf("\n");
        }

        if(payloadLength > 0) {
            printf("## Payload: \n");
            printf("### Payload Length: %zu\n", payloadLength);
            printRange(request.buffer, request.payloadStartIndex, request.dataLength);
        } else {
            printf("## No Payload\n");
        }
        printf("\n\n");

        if(payloadLength > 0) {
            char* body = Request_getPayload(&request);
            write(incomingSocket, "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: ", 57);
            write(incomingSocket, contentLength, strlen(contentLength));
            write(incomingSocket, "\n\n", 2);
            write(incomingSocket, body, strlen(body));
            free(body);
        } else {
            write(incomingSocket, "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 7\n\nHi Mom!", 67);
        }
        
        Request_reset(&request);
        free(contentLength);
        close(incomingSocket);
    }

    return 0;
}