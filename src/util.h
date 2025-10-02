#ifndef UTIL_H
#define UTIL_H

#include "data.h"

char* mkstring(char* data, size_t len);

void printstr(const char* format, char* data, size_t len);

char* format_string(const char *fmt, ...);

void request_debug(request_t* req);

int get_until(char* buff, char c, int len);

#endif
