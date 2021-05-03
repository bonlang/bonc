#include "helper.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define RED "\033[0;31m"
#define BLUE "\033[0;34m"
#define RESET "\033[0m"

void log_err(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, RED "error" RESET ": ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
}

void log_err_final(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, RED "error" RESET ": ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}

void log_internal_err(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, BLUE "internal error" RESET ": ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}
