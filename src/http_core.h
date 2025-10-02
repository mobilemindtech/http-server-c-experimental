#ifndef HTTP_CORE_H
#define HTTP_CORE_H

#include "http_parser.h"

void send_request(int* clientfd, request_t* req, response_t* resp);

void handle(void* arg);

void serve_blocking();

void setnonblocking(int sockfd);


#endif
