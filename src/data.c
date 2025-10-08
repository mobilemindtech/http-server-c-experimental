#include "data.h"

request_t* request_new(){
    request_t* req = malloc(sizeof(request_t));
    req->body_len = 0;
    req->headers_len = 0;
    req->queries_len = 0;
    return req;
}

int request_has_body(request_t* req) {
    return req->body_len > 0;
}

map_t* find_header(map_t* headers, int len, const char* name){

  map_t* pt = headers;
  int nlen = strlen(name);

  for(int i = 0; i < len; i++){
    if(headers[i].lkey == nlen && strncmp(headers[i].key, name, nlen) == 0){
      return &headers[i];
    }
  }

  return NULL;
}

response_t* response_new(){
    response_t* resp = malloc(sizeof(response_t));
    resp->body_len = 0;
    resp->headers_len = 0;
    return resp;
}

map_t* header_new(){
    map_t* header = malloc(sizeof(map_t));
    header->lkey = 0;
    header->lval = 0;
    return header;
}

