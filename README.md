# Hi There
This is an implementation of http request parser in C. Parser function named **Request_parseSocket()** and it doesn't use any dynamic memory.

I was too lazy to handle Hash Table collisions so headers and params can override each other.

Also Headers arrays are not implemented, but it can be easily implemented with special collision strategy: u don't replace items with the same key, u just add another item;

# Basic HTTP Request Structure:
METHOD PATH HTTP_VERSION\r\n

HEADER_1_NAME: HEADER_1_VALUE\r\n

HEADER_2_NAME: HEADER_2_VALUE\r\n

\r\n

PAYLOAD...

# Test Server
First u need to build stuff:
> make build

Then u can start the server:
> ./server

By default it will start on port 2000, but u can provide specific port with second parameter:
> ./server 2020

# Refs
 - [Repo](https://github.com/Astroner/basic-c-http-request-parser) explaining all stuff related to request method and path parsing
 - [ASCII codes](https://www.cs.cmu.edu/~pattis/15-1XX/common/handouts/ascii.html)