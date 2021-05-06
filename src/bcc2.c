#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "helper.h"
#include "ir_gen.h"
#include "lexer.h"
#include "parser.h"
#include "semantics.h"
#include "ssa.h"

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

  int in_fd = open(in_filename, O_RDONLY);
  if (in_fd == -1) {
    log_err_final("unable to open '%s'", in_filename);
  }

  struct stat _in_stat;
  if (fstat(in_fd, &_in_stat) == -1) {
    log_err_final("unable to get stats on '%s'", in_filename);
  }
  size_t in_size = _in_stat.st_size;

  const uint8_t* in_file =
      mmap(NULL, in_size, PROT_READ, MAP_PRIVATE, in_fd, 0);
  if (in_file == MAP_FAILED) {
    log_err_final("unable to get contents of '%s'", in_filename);
  }

  lexer_init(in_file, in_size);

  AST ast = parse_ast(in_file);
  resolve_names(&ast);
  resolve_types(&ast);
  check_returns(&ast);

  printf("AST_DUMP:\n");
  ast_dump(stdout, &ast);
  printf("\n");

  SSA_Prog ssa_prog;
  translate_ast(&ast, &ssa_prog, &ast.pool);

  printf("IR_DUMP:\n");
  ssa_prog_dump(stdout, &ssa_prog);

  ast_deinit(&ast);

  munmap((uint8_t*)in_file, in_size);
  return EXIT_SUCCESS;
}
