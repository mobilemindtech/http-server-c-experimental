#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "util.h"

void parse_request(char* buf, int* len, request_t* req, int *ret);

void buffer_decode(char *buf, int* buf_len);

void parse_url(request_t* req);

int hex_to_int(char* c);


#endif
