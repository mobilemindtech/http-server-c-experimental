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
#define MAX_HEADERS 10
#define true 1
#define false 0

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
} response_t;

request_t* request_new(){
  request_t* req = malloc(sizeof(request_t));
  return req;
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

void printstr(const char* format, char *data, size_t len) {
  char *str = malloc(len + 1);
  if (!str) {
    perror("malloc");
    return;
  }
  memcpy(str, data, len);  // copia os bytes
  str[len] = '\0';         // garante terminador
  printf(format, str);
  free(str);  // libera memória
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
      if(buff[i] == ' '){
        req->method_len = i - start;
	parser = PATH;
        start  = i+1;
        req->path = buff+start;
      }
      break;
    case PATH:
      if(buff[i] == ' '){
        req->path_len = i - start;
        parser = VERSION_IGNORE;
        start  = i+1;
      }
      break;
    case VERSION_IGNORE:
      if(buff[i] == '/'){
        parser = VERSION;
        start  = i+1;
        req->version = buff+start;
      }
      break;
    case VERSION:
      if(buff[i] == '\n'){
        req->version_len = i - start;
	parser = HEADER_NAME;
        start  = i+1;
        req->headers[++hlen].name = buff+start;
      }
      break;
    case HEADER_NAME:
      if(buff[i] == ':'){
        req->headers[hlen].name_len = i - start;
	parser = HEADER_VALUE;
        start  = i+1;
        req->headers[hlen].value = buff+start;
      }
      break;
    case HEADER_VALUE:
      if(buff[i] == '\n'){
        req->headers[hlen].value_len = i - start;
        if(buff[i+1] == '\r' || buff[i+1] == '\n') {
          i+= buff[i+1] == '\r' ? 2 : 1;
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

void send_request(int* clientfd, response_t* resp){
  char* raw_resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nHello!\r\n\r\n\0";
  send(*clientfd, raw_resp, strlen(raw_resp), 0);
}

void print_request(request_t* req){
  printf("::> request\n");
  printstr("::> method %s\n", req->method, req->method_len);
  printstr("::> path %s\n", req->path, req->path_len);
  printstr("::> version %s\n", req->version, req->version_len);
  for(int i = 0; i < req->headers_len; i++){
    printstr("::> %s", req->headers[i].name, req->headers[i].name_len);
    printstr("=%s\n", req->headers[i].value, req->headers[i].value_len);
  }
  printstr("::> body %s", req->body, req->body_len);
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
      print_request(req);

      response_t* resp = response_new();
      resp->body_len = req->body_len;
      resp->body = req->body;
      resp->headers[0].name = "Content-Type";
      resp->headers[0].name_len = 12;
      resp->headers[0].value = "text/plan";
      resp->headers[0].name_len = 9;
      resp->headers_len = 2;
      
      send_request(&clientfd, resp);
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
