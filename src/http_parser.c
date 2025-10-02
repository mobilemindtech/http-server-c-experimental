#include "http_parser.h"

int hex_to_int(char* c) {
  if ('0' <= *c && *c <= '9') return *c - '0';
  if ('A' <= *c && *c <= 'F') return *c - 'A' + 10;
  if ('a' <= *c && *c <= 'f') return *c - 'a' + 10;
  return -1;
}

void buffer_decode(char *buf, int* buf_len) {
  char *src = buf;
  char *dst = buf;
  char *end = buf + *buf_len;
  while (src < end) {
    if ((end - src) >= 3 && *src == '%' && isxdigit((unsigned char)*(src+1)) && isxdigit((unsigned char)*(src+2))) {
      int hi = hex_to_int(src+1);
      int lo = hex_to_int(src+2);
      if (hi >= 0 && lo >= 0) {
        *dst++ = (char)((hi << 4) | lo);
        src += 3;  // pula o %xx
        *buf_len-=2;
        continue;
      }
    } else if (*src == '+') {
      *dst++ = ' ';  // '+' vira espaço
      src++;
      continue;
    }
    *dst++ = *src++;
  }
  //*dst = '\0';  // finaliza a string
  //*buf_len = dst - buf;
}

void parse_url(request_t* req){
  char* buf = req->path;
  size_t len = req->path_len;
  enum PathParser parser;
  int qlen = -1;
  int start = 0;
  for(int i = 0; i < len; i++){
    if(buf[i] == '?'){
      parser = QUERY_NAME;
      req->queries[++qlen].key = &buf[++i];
      continue;
    }

    switch(parser){
    case QUERY_NAME:
    if(buf[i] == '='){
      req->queries[++qlen].lkey = i-start;
      start = i+1;
      req->queries[qlen].val = buf + start;
      parser = QUERY_VALUE;
    }
    break;
    case QUERY_VALUE:
    if(buf[i] == '&' || (buf[i] == CR || buf[i] == LF)){
      req->queries[qlen].lval = i - start;

      if(buf[i] != '&') break;

      start = i+1;
      req->queries[++qlen].key = buf + start;
      parser = QUERY_NAME;
    }
    break;
    }
  }

  req->queries_len = qlen;
}

void parse_request(char* buf, int* len, request_t* req, int *ret){


  enum HttpParser parser = METHOD;
  int hlen = -1;
  req->method = buf;
  int start = 0;

  for(int i = 0; i < *len; i++){

    switch(parser) {
    case METHOD:
      if (buf[i] == SPACE) {
        req->method_len = i - start;
	parser = PATH;
        start  = i+1;
        req->path = buf+start;
      }
      break;
    case PATH:
      if(buf[i] == SPACE){
        req->path_len = i - start;
        parser = VERSION;
      }
      break;
    case VERSION:

      CHECK_CHAR(&buf[i], 'H', INVALID_VERSION);
      CHECK_CHAR(&buf[++i], 'T', INVALID_VERSION);
      CHECK_CHAR(&buf[++i], 'T', INVALID_VERSION);
      CHECK_CHAR(&buf[++i], 'P', INVALID_VERSION);
      CHECK_CHAR(&buf[++i], '/', INVALID_VERSION);
      CHECK_CHAR(&buf[++i], '1', INVALID_VERSION);
      CHECK_CHAR(&buf[++i], '.', INVALID_VERSION);
      IS_DIGIT_CHAR(&buf[++i], INVALID_VERSION);

      req->minor_version = buf[i] - '0';
      req->version = &buf[i-2];
      req->version_len = 3;

      for(;;)
        if(buf[++i] == LF) break;


      parser = HEADER_NAME;
      start  = i+1;
      req->headers[++hlen].key = buf+start;

      break;
    case HEADER_NAME:
      if(buf[i] == COLON || buf[i] == SPACE){

        req->headers[hlen].lkey = i - start;

        if(buf[i] == SPACE) {
          for(;;) // ignore space between header name and colon
            if(buf[++i] == COLON) break;
        }

        parser = HEADER_VALUE;
        start  = i+2;
        req->headers[hlen].val = buf+start;
        continue;
      }

      // Em HTTP/1.x, campos como nomes de cabeçalhos (ex.: Content-Type, Host, etc.)
      // só podem conter caracteres do conjunto tchar definido na RFC 7230
      if(!header_name_char_map[(unsigned char)buf[i]]){
        *ret = INVALID_HEADER_NAME_CHAR;
        return;
      }
      break;
    case HEADER_VALUE:
      if(buf[i] == LF){
        req->headers[hlen].lval = (i - start) - 1;
        if(buf[i+1] == CR || buf[i+1] == LF) {
          i+= buf[i+1] == CR ? 2 : 1;
          start  = i+1;
	  parser = BODY;
          req->body = buf+start;
        } else {
          start  = i+1;
	  parser = HEADER_NAME;
          req->headers[++hlen].key = buf+start;
	}        
      }
      break;
    case BODY:
      // ignore
      break;
    }
 }

  req->body_len = *len - start;
  req->headers_len = hlen;
}
