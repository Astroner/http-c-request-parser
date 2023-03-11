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

typedef enum ParsingStage {
    ParsingStageMethod,
    ParsingStagePath,
    ParsingStageParams,
    ParsingStageHeaders,
} ParsingStage;

int Request_parseSocket(int socket, Request* result) {
    size_t dataLength = read(socket, result->buffer, result->bufferSize);

    result->dataLength = dataLength;

    ParsingStage stage = ParsingStageMethod;

    int guessDoubleNL = 0;
    int gotMethodFromFirstChar = 0;

    size_t headerKeyStart;
    size_t headerKeyEnd;
    size_t headerValueStart;
    
    size_t paramKeyStart = 0;
    size_t paramKeyEnd = 0;
    size_t paramValueStart = 0;

    for(size_t i = 0; i < dataLength; i++) {
        if(i == 0) {
            if(result->buffer[0] == CHAR_G) {
                result->method = RequestMethodGet;
                gotMethodFromFirstChar = 1;
                continue;
            } else if(result->buffer[0] == CHAR_D) {
                result->method = RequestMethodDelete;
                gotMethodFromFirstChar = 1;
                continue;
            } else if(result->buffer[0] == CHAR_P) {
                continue;
            } else {
                return -1;
            }
        } else if(i == 1) {
            if(gotMethodFromFirstChar) {
                continue;  
            } else if(result->buffer[1] == CHAR_O) {
                result->method = RequestMethodPost;
                continue;
            } else if(result->buffer[1] == CHAR_U) {
                result->method = RequestMethodPut;
                continue;
            } else if(result->buffer[1] == CHAR_A) {
                result->method = RequestMethodPatch;
                continue;
            } else {
                return -2;
            }
        } else if(result->buffer[i] == CHAR_SPACE) {
            if(stage == ParsingStageMethod) {
                result->originalUrl.start = i + 1;
                stage = ParsingStagePath;
            } else if(stage == ParsingStagePath) {
                result->originalUrl.end = i - 1;
            } else if(stage == ParsingStageParams) {
                if(paramValueStart) {
                    HashTable_set(result->params, paramKeyStart, paramKeyEnd, paramValueStart, i - 1);   
                }
                result->originalUrl.end = i - 1;
            } else if(stage == ParsingStageHeaders) {
                continue;
            } else {
                return -3;
            }
        } else if(result->buffer[i] == CHAR_NL) {
            if(stage == ParsingStagePath || stage == ParsingStageParams) {
                stage = ParsingStageHeaders;
                headerKeyStart = i + 1;
            } else if(stage == ParsingStageHeaders) {
                if(guessDoubleNL) {
                    result->payloadStartIndex = i + 1;
                    return 0;
                } else {
                    guessDoubleNL = 1;
                    HashTable_set(result->headers, headerKeyStart, headerKeyEnd, headerValueStart, i - 1);
                    headerKeyStart = i + 1;
                }
            } else {
                return -4;
            }
        } else if(result->buffer[i] == CHAR_COLON) {
            if(stage == ParsingStageHeaders) {
                headerKeyEnd = i - 1;
                headerValueStart = i + 2;
            } else {
                return -5;
            }
        } else if(result->buffer[i] == CHAR_AMP) {
            if(stage == ParsingStageParams) {
                HashTable_set(result->params, paramKeyStart, paramKeyEnd, paramValueStart, i - 1);
                paramKeyStart = i + 1;
                paramKeyEnd = 0;
                paramValueStart = 0;
            } else {
                continue;
            }
        } else if(result->buffer[i] == CHAR_EQUAL) {
            if(stage == ParsingStageParams) {
                paramKeyEnd = i - 1;
                paramValueStart = i + 1;
            } else {
                continue;
            }
        } else if (result->buffer[i] == CHAR_QUESTION_MARK) {
            result->path.start = result->originalUrl.start;
            result->path.end = i - 1;
            stage = ParsingStageParams;
            paramKeyStart = i + 1;
        } else if(
            (result->buffer[i] >=65 && result->buffer[i] <=90) /* [A-Z] */ 
            || 
            (result->buffer[i] >=97 && result->buffer[i] <= 122) /* [a-z] */
        ) {
            guessDoubleNL = 0;
        };
    }
        
    return 0;
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
    if(request->dataLength - request->payloadStartIndex) {
        return extractRange(request->buffer, request->payloadStartIndex, request->dataLength);
    }
    return NULL;
}

void Request_reset(Request* request) {
    memset(request->buffer, 0, request->bufferSize);
    HashTable_clear(request->headers);
    HashTable_clear(request->params);

    request->dataLength = 0;

    request->originalUrl.start = 0;
    request->originalUrl.end = 0;

    request->path.start = 0;
    request->path.end = 0;

    request->payloadStartIndex = 0;
}
