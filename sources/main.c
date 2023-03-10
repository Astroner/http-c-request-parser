#include "main.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "HashTable.h"

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
        .sin_port = htons(2000),
    };

    if(bind(mainSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) return 1;

    if(listen(mainSocket, 3) < 0) return 2;

    int incomingSocket;
    struct sockaddr_in incomingAddress;
    socklen_t incomingAddressLength;

    while(1) {
        if((incomingSocket = accept(mainSocket, (struct sockaddr*)&incomingAddress, &incomingAddressLength)) < 0) return 3;

        write(incomingSocket, "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 7\n\nHi Mom!", 67);

        close(incomingSocket);
    }

    return 0;
}