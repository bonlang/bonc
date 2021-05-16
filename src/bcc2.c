#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "helper.h"
#include "ir_gen.h"
#include "lexer.h"
#include "parser.h"
#include "semantics.h"
#include "ssa.h"

struct {
  int ast_dump;
  int ir_dump;
  int reg_dump;
  int help;
  int version;
  const char *in_file;
} flags;

void
parse_args(int argc, char *argv[]) {
  memset(&flags, 0, sizeof(flags));
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      if (flags.in_file) {
        log_err_final("can't specify more than one input file");
      }
      flags.in_file = argv[i];
    } else {
      flags.ast_dump |= strcmp(argv[i], "-ast") == 0;
      flags.ir_dump |= strcmp(argv[i], "-ir") == 0;
      flags.reg_dump |= strcmp(argv[i], "-regs") == 0;
      flags.help |= strcmp(argv[i], "-h") == 0;
      flags.version |= strcmp(argv[i], "-v") == 0;
    }
  }
}

int
main(int argc, char *argv[]) {
  parse_args(argc, argv);

  if (!flags.in_file) {
    log_err_final("no input file specified");
  }

  if (!flags.ir_dump && flags.reg_dump) {
    log_err_final("cannot print registers without printing the IR");
  }

  if (flags.help) {
    printf("-h : prints help\n-v : prints version\n-ast : dumps ast to "
           "stdout\n-ir : dumps ir to stdout\n-regs : dumps registers to "
           "stdout\n");
    exit(EXIT_SUCCESS);
  }
  if (flags.version) {
    printf("bcc2 : v0.1\n");
    exit(EXIT_SUCCESS);
  }

  int in_fd = open(flags.in_file, O_RDONLY);
  if (in_fd == -1) {
    log_err_final("unable to open '%s'", flags.in_file);
  }

  struct stat _in_stat;
  if (fstat(in_fd, &_in_stat) == -1) {
    log_err_final("unable to get stats on '%s'", flags.in_file);
  }
  size_t in_size = _in_stat.st_size;

  const uint8_t *in_file =
      mmap(NULL, in_size, PROT_READ, MAP_PRIVATE, in_fd, 0);
  if (in_file == MAP_FAILED) {
    log_err_final("unable to get contents of '%s'", flags.in_file);
  }

  lexer_init(in_file, in_size);

  AST ast = parse_ast(in_file);
  resolve_names(&ast);
  resolve_types(&ast);
  check_returns(&ast);

  if (flags.ast_dump) {
    printf("AST_DUMP:\n");
    ast_dump(stdout, &ast);
    printf("\n");
  }

  SSA_Prog ssa_prog;
  translate_ast(&ast, &ssa_prog);

  if (flags.ir_dump) {
    printf("IR_DUMP:\n");
    ssa_prog_dump(stdout, &ssa_prog, flags.reg_dump);
  }

  ast_deinit(&ast);

  munmap((uint8_t *)in_file, in_size);
  return EXIT_SUCCESS;
}
