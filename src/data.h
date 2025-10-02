#ifndef DATA_H
#define DATA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <ctype.h>

#define ERROR_EXIT(msg) \
    fprintf(stderr, "ERROR in %s:%d - %s\n", __FILE__, __LINE__, msg); \
    exit(EXIT_FAILURE);

#define ERROR_EXIT_IF(test, msg) \
    if (test) {\
        fprintf(stderr, "ERROR in %s:%d - %s\n", __FILE__, __LINE__, msg); \
        exit(EXIT_FAILURE); \
    }

#define CHECK_CHAR(c, expect, err) \
    if(*c != expect) {\
        *ret = err;\
        return;\
    }

#define IS_DIGIT_CHAR(c, err) \
    if (c < '0' && c > '9') {\
        *ret = err;\
        return;\
    }

#define PORT 8080
#define BUF_SIZE 1024
#define MAX_HEADERS 100
#define MAX_QUERIES 100
#define HTTP_VERSION "1.1"
#define true 1
#define false 0
#define LF '\n'
#define CR '\r'
#define SPACE ' '
#define COLON ':'
#define SLASH '/'

// errors

#define INVALID_HEADER_NAME_CHAR -1
#define INVALID_VERSION -2

static const char *header_name_char_map = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                          "\0\1\0\1\1\1\1\1\0\0\1\1\0\1\1\0\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0"
                                          "\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\1\1"
                                          "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\1\0"
                                          "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                          "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                          "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                          "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

static const char *path_char_map = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                   "\0\1\0\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\0\1"
                                   "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\0\1\0"
                                   "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\1\0"
                                   "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1"
                                   "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1"
                                   "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1"
                                   "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1";

static const char *header_value_char_map = "\0\0\0\0\0\0\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1"
                                           "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1"
                                           "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1"
                                           "\1\1\1\1\1\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1"
                                           "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1"
                                           "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1"
                                           "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1";

enum HttpParser {
    METHOD, PATH, VERSION, HEADER_NAME, HEADER_VALUE, BODY
};

enum PathParser {
    QUERY_NAME, QUERY_VALUE
};

typedef struct {
    char* key;
    size_t lkey;
    char* val;
    size_t lval;
} map_t;

typedef struct {
    char* method;
    size_t method_len;
    char* path;
    size_t path_len;
    char* version;
    size_t version_len;
    int minor_version;
    map_t headers[MAX_HEADERS];
    size_t headers_len;
    map_t queries[MAX_QUERIES];
    size_t queries_len;
    char* body;
    size_t body_len;
} request_t;

typedef struct {
    map_t headers[MAX_HEADERS];
    size_t headers_len;
    char* body;
    size_t body_len;
    int status_code;
} response_t;

// request_t
request_t* request_new();
int request_has_body(request_t* req);

// response_t
response_t* response_new();

#endif
