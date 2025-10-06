#include "http_parser.h"

#define EOF() (buf[i] == CR || buf[i] == LF || buf[i] == '\0')

#define ENDFOR() (i == len-1)

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

/**
 * @brief parse_url Parse URL,
 *
 * Extracting query params.
 * Change curr path len to ignore queries
 *
 * @param req request to parse
 */
void parse_url(request_t* req){
  char* buf = req->path;
  size_t len = req->path_len;
  enum PathParser parser = NONE;
  int qlen = -1;
  int curr_start_idx = 0;

  for(int i = 0; i < len; i++){
    if(buf[i] == '?'){

      // change curr path len to ignore query
      req->path_len = i;

      parser = QUERY_NAME;
      curr_start_idx = i+1;

      if(!ENDFOR()) // check no query
        req->queries[++qlen].key = buf[i+1] == '=' ? NULL : &buf[++i];

      continue;
    }

    switch(parser){
    case QUERY_NAME:
    if(buf[i] == '=' || EOF() || ENDFOR()){
      req->queries[qlen].lkey = i-curr_start_idx;

      if(!EOF() && ENDFOR() && buf[i] != '=')
        req->queries[qlen].lkey += 1;

      if(buf[i] != '=') goto done;

      curr_start_idx = i+1;
      req->queries[qlen].val = buf + curr_start_idx;
      parser = QUERY_VALUE;
    }
    break;
    case QUERY_VALUE:

    if(buf[i] == '&' || EOF() || ENDFOR()){
      req->queries[qlen].lval = i-curr_start_idx;

      if(!EOF() && ENDFOR() && buf[i] != '&') // precisa incrementar no último caracter
        req->queries[qlen].lval+=1;

      if(buf[i] != '&')  goto done;

      curr_start_idx = i+1;
      req->queries[++qlen].key = buf + curr_start_idx;
      parser = QUERY_NAME;
    }
    break;
    }
  }

  done:
  req->queries_len = qlen > -1 ? qlen+1 : 0;
}

/**
 * @brief parse_request
 * @param buf
 * @param len
 * @param req
 * @param ret
 */
void parse_request(char* buf, int* len, request_t* req, int *ret){

  enum HttpParser parser = METHOD;
  int hlen = -1;
  req->method = buf;
  int last_idx = 0;

  for(int i = 0; i < *len; i++){

    switch(parser) {
    case METHOD:
      if (buf[i] == SPACE) {
        req->method_len = i - last_idx;
	parser = PATH;
        last_idx  = i+1;
        req->path = buf+last_idx;
      }
      break;
    case PATH:
      if(buf[i] == SPACE){
        req->path_len = i - last_idx;
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
      last_idx  = i+1;
      req->headers[++hlen].key = buf+last_idx;

      break;
    case HEADER_NAME:
      if(buf[i] == COLON){

        if(buf[i-1] == SPACE) {
          int j = i;
          for(;;) // ignore space between header name and colon
            if(buf[--i] != SPACE) break;
          req->headers[hlen].lkey = j+1 - last_idx;
        }else{
          req->headers[hlen].lkey = i - last_idx;
        }


        parser = HEADER_VALUE;
        last_idx  = i+2;
        req->headers[hlen].val = buf+last_idx;
        continue;
      }

      if(buf[i] == CR || buf[i] == LF){
        *ret = INVALID_HEADER;
        return;
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
        req->headers[hlen].lval = (i - last_idx) - 1;
        if(buf[i+1] == CR || buf[i+1] == LF) {
          i+= buf[i+1] == CR ? 2 : 1;
          last_idx  = i+1;
	  parser = BODY;
          req->body = buf+last_idx;
        } else {
          last_idx  = i+1;
	  parser = HEADER_NAME;
          req->headers[++hlen].key = buf+last_idx;
	}        
      }
      break;
    case BODY:
      // ignore
      break;
    }
 }

  req->body_len = *len - last_idx;
  req->headers_len = hlen > -1 ? hlen+1 : hlen;
}
