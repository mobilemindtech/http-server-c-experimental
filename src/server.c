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

#define ERROR_EXIT(msg) \
    fprintf(stderr, "ERROR in %s:%d - %s\n", __FILE__, __LINE__, msg); \
    exit(EXIT_FAILURE);

#define ERROR_EXIT_IF(test, msg) \
    if (test) {\
      fprintf(stderr, "ERROR in %s:%d - %s\n", __FILE__, __LINE__, msg); \
      exit(EXIT_FAILURE); \
    }

#define PORT 8080
#define BUF_SIZE 1024
#define MAX_HEADERS 100
#define HTTP_VERSION "1.1"
#define true 1
#define false 0
#define LF '\n'
#define CR '\r'
#define SPACE ' '
#define COLON ':'
#define SLASH '/'

enum Parser {
  METHOD, PATH, VERSION_IGNORE, VERSION, HEADER_NAME, HEADER_VALUE, BODY
};

typedef struct {
  char* name;
  size_t name_len;
  char* value;
  size_t value_len;
} header_t;

typedef struct {
  char* method;
  size_t method_len;
  char* path;
  size_t path_len;
  char* version;
  size_t version_len;
  header_t headers[MAX_HEADERS];
  size_t headers_len;
  char* body;
  size_t body_len;
} request_t;

typedef struct {
  header_t headers[MAX_HEADERS];
  size_t headers_len;
  char* body;
  size_t body_len;
  int status_code;
} response_t;

request_t* request_new(){
  request_t* req = malloc(sizeof(request_t));
  return req;
}

int request_has_body(request_t* req) {
  return req->body_len > 0;
}

response_t* response_new(){
  response_t* resp = malloc(sizeof(response_t));
  return resp;
}

header_t* header_new(){
  header_t* header = malloc(sizeof(header_t));
  return header;
}

void setnonblocking(int sockfd){
  int flags = fcntl(sockfd, F_GETFL, 0);
  ERROR_EXIT_IF(flags == -1, "fcntl(F_GETFL)");
  ERROR_EXIT_IF(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1, "fcntl(F_SETFL)");

  int opt = 1;
  ERROR_EXIT_IF(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0, "setsockopt(SO_REUSEADDR)");
  
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
  printf("DEBUG ::> [%ld]version %s\n", req->version_len, version);
  printf("DEBUG ::> headers %ld\n", req->headers_len);
  for(int i = 0; i < req->headers_len; i++){
    char* name = mkstring(req->headers[i].name, req->headers[i].name_len);
    char* value = mkstring(req->headers[i].value, req->headers[i].value_len);
    printf("DEBUG ::> [%ld]%s=[%ld]%s\n", req->headers[i].name_len, name, req->headers[i].value_len, value);
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

request_t* parse_request(char* buff, int* len){
  request_t* req = request_new();

  enum Parser parser = METHOD;
  int hlen = -1;
  req->method = buff;
  int start = 0;

  for(int i = 0; i < *len; i++){


    switch(parser) {
    case METHOD:
      if (buff[i] == SPACE) {
        req->method_len = i - start;
	parser = PATH;
        start  = i+1;
        req->path = buff+start;
      }
      break;
    case PATH:
      if(buff[i] == SPACE){
        req->path_len = i - start;
        parser = VERSION_IGNORE;
      }
      break;
    case VERSION_IGNORE:
      if(buff[i] == SLASH){
        parser = VERSION;
        start  = i+1;
        req->version = buff+start;
      }
      break;
    case VERSION:
      if(buff[i] == LF){
        req->version_len = (i - start) - 1;
	parser = HEADER_NAME;
        start  = i+1;
        req->headers[++hlen].name = buff+start;
      }
      break;
    case HEADER_NAME:
      if(buff[i] == COLON){
        req->headers[hlen].name_len = i - start;
        parser = HEADER_VALUE;
        start  = i+2;
        req->headers[hlen].value = buff+start;
      }
      break;
    case HEADER_VALUE:
      if(buff[i] == LF){
        req->headers[hlen].value_len = (i - start) - 1;
        if(buff[i+1] == CR || buff[i+1] == LF) {
          i+= buff[i+1] == CR ? 2 : 1;
          start  = i+1;
	  parser = BODY;
          req->body = buff+start;
        } else {
          start  = i+1;
	  parser = HEADER_NAME;
          req->headers[++hlen].name = buff+start;
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

  return req;
  //while((l = get_until(pt, '\n', max)) > 0) {
  //  write(1, pt, l);
  //  max -= l;		  
  //  pt += l;
  //}
}

void send_request(int* clientfd, request_t* req, response_t* resp){

  char* body_raw = mkstring(resp->body, resp->body_len);
  char* body = request_has_body(req) ? format_string("\r\n%s\r\n", body_raw) : NULL;
  char* content_type_default = "Content-Type: text/plain\r\n";
  char* content_length = format_string("Content-Length: %ld\r\n", resp->body_len);
  char* content_type = NULL;

  for(int i = 0; i < req->headers_len; i++){
    char* name = mkstring(req->headers[i].name, req->headers[i].name_len);
    printf("compare %s / %ld\n", name, req->headers[i].name_len);
    if(strcmp("Content-Type", name) == 0){
      char* value = mkstring(req->headers[i].value, req->headers[i].value_len);
      content_type = format_string("%s: %s\r\n", name, value);
      free(value);
      free(name);
      break;
    }
    free(name);
  }


  char* respstr = format_string(
    "HTTP/%s %d OK\r\n%s%s%s",
    HTTP_VERSION ,
    resp->status_code,
    content_type == NULL ? content_type_default : content_type,
    content_length,
    body == NULL ? "" : body);

  send(*clientfd, respstr, strlen(respstr), 0);

  free(body_raw);
  if(body)
    free(body);
  free(respstr);
  free(content_length);
  if(content_type)
    free(content_type);
}

void handle(void* arg){
  int clientfd = *((int *) arg);
  while(true) {
    char buff[BUF_SIZE];
    int len = recv(clientfd, buff, BUF_SIZE, 0);
    if (len == 0) {
      return;
    } else if (len == -1) {
      ERROR_EXIT("read error");
    } else {
      request_t* req = parse_request(buff, &len);
      request_debug(req);

      response_t* resp = response_new();
      resp->status_code = 200;
      resp->body_len = req->body_len;
      resp->body = req->body;

      
      send_request(&clientfd, req, resp);
      free(req);
      free(resp);
      
      close(clientfd);
      break;
    }
  }  
}

void serve_blocking() {


  int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  ERROR_EXIT_IF(sockfd < 0, "error while creating socket");

  //setnonblocking(sockfd);

  struct sockaddr_in addr;
  //memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  ERROR_EXIT_IF(bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0, "error in socket bind");

  socklen_t addrlen = sizeof(struct sockaddr);

  ERROR_EXIT_IF(listen(sockfd, 10) < 0, "error in socket listen");

  printf("server listern on http://localhost:%d\n", PORT);

  while(true) {

    int clientfd = accept(sockfd, (struct sockaddr *)&addr, &addrlen);

    ERROR_EXIT_IF(clientfd < 0, "error in socket accept");


    // handle with threads
    //pthread_t thread_id;
    //pthread_create(&thread_id, NULL, handle_client, (void *)client_fd);
    //pthread_detach(thread_id);
    handle(&clientfd);
  }

  close(sockfd);
  printf("\nsever close!\n");

}

int main() {
  setvbuf(stdout, NULL, _IONBF, 0);
  signal(SIGPIPE, SIG_IGN);
  printf("app serve\n");
  serve_blocking();
  return 0;
}
