#include "http_core.h"

void setnonblocking(int sockfd){
  int flags = fcntl(sockfd, F_GETFL, 0);
  ERROR_EXIT_IF(flags == -1, "fcntl(F_GETFL)");
  ERROR_EXIT_IF(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1, "fcntl(F_SETFL)");
  int opt = 1;
  ERROR_EXIT_IF(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0, "setsockopt(SO_REUSEADDR)");
}


void send_request(int* clientfd, request_t* req, response_t* resp){

  char* body_raw = mkstring(resp->body, resp->body_len);
  char* body = request_has_body(req) ? format_string("\r\n%s\r\n", body_raw) : NULL;
  char* content_type_default = "Content-Type: text/plain\r\n";
  char* content_length = format_string("Content-Length: %ld\r\n", resp->body_len);
  char* content_type = NULL;

  for(int i = 0; i < req->headers_len; i++){
    char* name = mkstring(req->headers[i].key, req->headers[i].lkey);
    printf("compare %s / %ld\n", name, req->headers[i].lkey);
    if(strcmp("Content-Type", name) == 0){
      char* value = mkstring(req->headers[i].val, req->headers[i].lval);
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
    char buf[BUF_SIZE];
    int len = recv(clientfd, buf, BUF_SIZE, 0);
    if (len == 0) {
      return;
    } else if (len == -1) {
      ERROR_EXIT("read error");
    } else {
      int ret = 0;
      request_t* req = request_new();
      parse_request(buf, &len, req, &ret);
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
