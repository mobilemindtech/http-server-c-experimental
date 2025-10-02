
#include "http_core.h"



int main() {
  setvbuf(stdout, NULL, _IONBF, 0);
  signal(SIGPIPE, SIG_IGN);
  printf("app serve\n");
  serve_blocking();
  return 0;
}
