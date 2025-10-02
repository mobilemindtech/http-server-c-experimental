#include "data.h"

request_t* request_new(){
    request_t* req = malloc(sizeof(request_t));
    req->headers_len = 0;
    return req;
}

int request_has_body(request_t* req) {
    return req->body_len > 0;
}

response_t* response_new(){
    response_t* resp = malloc(sizeof(response_t));
    return resp;
}

map_t* header_new(){
    map_t* header = malloc(sizeof(map_t));
    return header;
}
