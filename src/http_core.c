#include "http_core.h"

void setnonblocking(int sockfd){
  int flags = fcntl(sockfd, F_GETFL, 0);
  ERROR_EXIT_IF(flags == -1, "fcntl(F_GETFL)");
  ERROR_EXIT_IF(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1, "fcntl(F_SETFL)");
  int opt = 1;
  ERROR_EXIT_IF(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0, "setsockopt(SO_REUSEADDR)");
}

void headers_add(response_t* resp, const char* name, const char* value){
  resp->headers[resp->headers_len].key = name;
  resp->headers[resp->headers_len].lkey = strlen(name);
  resp->headers[resp->headers_len].val = value;
  resp->headers[resp->headers_len].lval = strlen(value);
  resp->headers_len++;
}

void header_add_body_len(response_t* resp){
  char body_len[10];
  sprintf(body_len, "%ld", resp->body_len);
  resp->headers[resp->headers_len].key = "Content-Length";
  resp->headers[resp->headers_len].lkey = 14;
  resp->headers[resp->headers_len].val = body_len;
  resp->headers[resp->headers_len].lval = strlen(body_len);
  resp->headers_len++;
}

void mk_server_error(response_t* resp){
  resp->status_code = 500;
  resp->body = "Server error";
  resp->body_len = 12;
  headers_add(resp, "Content-Type", "text/plain");
  header_add_body_len(resp);
}

void send_request(int* clientfd, response_t* resp){

  char line[30];
  sprintf(line, "HTTP/%s %d OK\r\n", HTTP_VERSION, resp->status_code);
  send(*clientfd, line, strlen(line), 0);

  for(int i=0; i<resp->headers_len;i++){
    send(*clientfd, resp->headers[i].key, resp->headers[i].lkey, 0);
    send(*clientfd, ": ", 2, 0);
    send(*clientfd, resp->headers[i].val, resp->headers[i].lval, 0);
    send(*clientfd, "\r\n", 2, 0);
  }

  send(*clientfd, "\r\n", 2, 0);

  if(resp->body_len > 0){
    send(*clientfd, resp->body, resp->body_len, 0);
    send(*clientfd, "\r\n\r\n", 4, 0);
  }

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
      //request_debug(req);
      response_t* resp = response_new();

      if (ret != 0) {
        printf("ERROR: %d\n", ret);
        mk_server_error(resp);
      } else {
        map_t* content_type = find_header(req->headers, req->headers_len, "Content-Type");
        resp->status_code = 200;
        resp->body_len = req->body_len;
        resp->body = req->body;

        if(content_type != NULL){
          resp->headers[resp->headers_len].key = content_type->key;
          resp->headers[resp->headers_len].lkey = content_type->lkey;
          resp->headers[resp->headers_len].val = content_type->val;
          resp->headers[resp->headers_len].lval = content_type->lval;
          resp->headers_len++;
        } else {
          headers_add(resp, "Content-Type", "text/plain");
          //resp->headers[++hlen].key = "Content-Type";
          //resp->headers[hlen].lkey = 12;
          //resp->headers[hlen].val = "text/plain";
          //resp->headers[hlen].lval = 10;
        }

        header_add_body_len(resp);
      }

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

  ERROR_EXIT_IF(listen(sockfd, MAX_BACKLOG) < 0, "error in socket listen");

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
