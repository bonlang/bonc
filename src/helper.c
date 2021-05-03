#include "helper.h"

#include <stdarg.h>
#include <stdio.h>

#define RED "\033[0;31m"
#define RESET "\033[0m"

void log_err(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, RED "error" RESET ": ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
}
