#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "helper.h"

int main(int argc, char* argv[]) {
  int c, invalid_args = 0;

  opterr = 0;
  while ((c = getopt(argc, argv, "hv")) != -1) {
    switch (c) {
      case 'h':
        printf("-v : print version info\n-h : print help\n");
        exit(EXIT_SUCCESS);
      case 'v':
        printf("comp : simple compiler (v0.1)\n");
        exit(EXIT_SUCCESS);
      case '?':
        log_err("unknown argument '-%c'", optopt);
        invalid_args = 1;
        break;
    }
  }
  if (invalid_args) {
    exit(EXIT_FAILURE);
  }
  printf("Hello, comp\n");
  return EXIT_SUCCESS;
}
