#include "util.h"

char* mkstring(char* data, size_t len){
  char *str = malloc(sizeof(char) * (len + 1));
  if (!str) {
    perror("malloc");
    return NULL;
  }
  memcpy(str, data, len);  // copia os bytes
  str[len] = '\0';
  return str;
}

void printstr(const char* format, char* data, size_t len) {
  char *str = mkstring(data, len);
  memcpy(str, data, len);  // copia os bytes
  free(str);  // libera memória
}

char* format_string(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    // Descobre o tamanho necessário
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    if (needed < 0) {
        va_end(args);
        return NULL;
    }

    // Aloca memória suficiente
    char *buffer = malloc(needed + 1);
    if (!buffer) {
        va_end(args);
        return NULL;
    }

    // Preenche a string
    vsnprintf(buffer, needed + 1, fmt, args);
    va_end(args);

    return buffer;
}

void request_debug(request_t* req){
  char* method = mkstring(req->method, req->method_len);
  char* path = mkstring(req->path, req->path_len);
  char* version = mkstring(req->version, req->version_len);
  char* body = request_has_body(req) ? mkstring(req->body, req->body_len) : NULL;
  printf("DEBUG ::> request\n");
  printf("DEBUG ::> [%ld]method %s\n", req->method_len, method);
  printf("DEBUG ::> [%ld]path %s\n", req->path_len, path);
  printf("DEBUG ::> [%ld]version %s, minor version %d\n", req->version_len, version, req->minor_version);
  printf("DEBUG ::> headers %ld\n", req->headers_len);
  for(int i = 0; i < req->headers_len; i++){
    char* name = mkstring(req->headers[i].key, req->headers[i].lkey);
    char* value = mkstring(req->headers[i].val, req->headers[i].lval);
    printf("DEBUG ::> [%ld]%s=[%ld]%s\n", req->headers[i].lkey, name, req->headers[i].lval, value);
    free(name);
    free(value);
  }
  printf("DEBUG ::> queries %ld\n", req->queries_len);
  for(int i = 0; i < req->queries_len; i++){
    char* name = mkstring(req->queries[i].key, req->queries[i].lkey);
    char* value = mkstring(req->queries[i].val, req->queries[i].lval);
    printf("DEBUG ::> [%ld]%s=[%ld]%s\n", req->queries[i].lkey, name, req->queries[i].lval, value);
    free(name);
    free(value);
  }
  printf("DEBUG ::> [%ld]body %s\n", req->body_len, body);

  free(method);
  free(path);
  free(version);
  if(body != NULL)
    free(body);
}

int get_until(char* buff, char c, int len){
  for(int i = 0; i < len; i++){
    buff += 1;
    if(*buff == c){
      return i+1;
    }
  }
  return len;
}

