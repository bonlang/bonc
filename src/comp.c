#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "helper.h"

int main(int argc, char* argv[]) {
  int c, invalid_args = 0;

  const char* in_filename = NULL;
  opterr = 0;
  while ((c = getopt(argc, argv, "hvi:")) != -1) {
    switch (c) {
      case 'h':
        printf("-v : print version info\n-h : print help\n");
        exit(EXIT_SUCCESS);
      case 'v':
        printf("comp : simple compiler (v0.1)\n");
        exit(EXIT_SUCCESS);
      case 'i':
        in_filename = optarg;
        break;
      case '?':
        if (optopt == 'i') {
          log_err_final("expected filename after '-i'");
          break;
        }
        log_err("unknown argument '-%c'", optopt);
        invalid_args = 1;
        break;
    }
  }
  if (invalid_args) {
    exit(EXIT_FAILURE);
  }
  if (!in_filename) {
    log_err_final("no input file specified");
  }
  printf("%s\n", in_filename);

  return EXIT_SUCCESS;
}
