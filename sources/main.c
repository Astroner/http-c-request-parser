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

void printRange(char* buffer, size_t start, size_t end, PrintRangeMode mode) {
    for(size_t i = start; i <= end; i++) {
        switch(mode) {
            case PrintRangeModeDefault:
                printf("%c", buffer[i]);
                break;
            case PrintRangeModeWithCodes:
                printf("%c(%d)", buffer[i], buffer[i]);
                break;
            case PrintRangeModeCodesOnly:
                if(buffer[i] == 13 || buffer[i] == 10) printf("\n");
                printf("%d_", buffer[i]);
                if(buffer[i] == 13 || buffer[i] == 10) printf("\n");
                break;
        }
    }
}

int initSocket(uint16_t port, int listenQueueSize) {
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
        .sin_port = htons(port),
    };

    if(bind(mainSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) return -1;

    if(listen(mainSocket, listenQueueSize) < 0) return -2;

    return mainSocket;
}

int main(int argc, char** argv) {
    uint16_t port = 2000;

    if(argc > 1) {
        int arg = atoi(argv[1]);
        if(arg > 0) port = arg;
        else printf("# Invalid PORT argument\n");
    }

    int mainSocket = initSocket(port, 3);
    
    if(mainSocket < 0) {
        printf("Failed to init socket (%d)\n", mainSocket);
        return 1;
    }

    printf("# Server started on port %d\n", port);

    int incomingSocket;

    createStaticRequest(request, 2024, 100, 100);
    while(1) {
        if((incomingSocket = accept(mainSocket, NULL, NULL)) < 0) return 3;

        printf("# New Request:\n");
        int status;
        if((status = Request_parseSocket(incomingSocket, request)) < 0) {
            printf("Failed to parse request: (%d)\n", status);
            printRange(request->buffer, 0, request->dataLength, PrintRangeModeDefault);
            write(incomingSocket, "HTTP/1.1 400 Bad Request\n\r\n", 27);
            Request_reset(request);
            close(incomingSocket);

            continue;
        };
        Request_print(request);

        size_t payloadLength = Request_payloadLength(request);

        char* contentLength;
        if(!(contentLength = Request_getHeader(request, "Content-Length")) && payloadLength) {
            printf("## Response: 400\n");
            write(incomingSocket, "HTTP/1.1 400 Bad Request\nContent-Type: text/plain\nContent-Length: 26\n\nNo Content-Length provided", 96);
            close(incomingSocket);
            Request_reset(request);
            continue;
        }
        
        size_t contentLengthParsed = contentLength ? (size_t)atol(contentLength) : payloadLength;

        if(contentLengthParsed != payloadLength) {
            printf("## Response: 400\n");
            write(incomingSocket, "HTTP/1.1 400 Bad Request\nContent-Type: text/plain\nContent-Length: 20\n\nWrong Content Length", 90);
            close(incomingSocket);
            Request_reset(request);
            continue;
        }
        printf("## Response: 200\n");
        if(payloadLength > 0) {
            char* body = Request_getPayload(request);
            write(incomingSocket, "HTTP/1.1 200 OK\nContent-Type: text/plain\nConnection: close\nContent-Length: ", 75);
            write(incomingSocket, contentLength, strlen(contentLength));
            write(incomingSocket, "\n\n", 2);
            write(incomingSocket, body, strlen(body));
        } else {
            write(incomingSocket, "HTTP/1.1 200 OK\nContent-Type: text/plain\nConnection: close\nContent-Length: 7\n\nHi Mom!", 85);
        }
        
        Request_reset(request);
        free(contentLength);
        close(incomingSocket);
    }

    return 0;
}