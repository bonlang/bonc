#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "args.h"
#include "helper.h"
#include "ir_gen.h"
#include "lexer.h"
#include "parser.h"
#include "semantics.h"
#include "ssa.h"

void print_help(const char *program_name, const char *program_description, struct Option *opts[], size_t opts_size) {
  printf("%s: %s\n", program_name, program_description);
  printf("Usage: %s [OPTION]... [FILE]\n", program_name);
  print_flags(opts, opts_size);
}

int
main(int argc, char *argv[]) {
  struct Option help = {
      .flag = "h",
      .description = "prints the help message",
      .required_arg = ARG_NONE,
      .type = OPT_BOOLEAN,
      .long_flag = false,
  };
  struct Option version = {
      .flag = "v",
      .description = "prints the version",
      .required_arg = ARG_NONE,
      .type = OPT_BOOLEAN,
      .long_flag = false,
  };
  struct Option ast_dump_flag = {
      .flag = "dump-ast",
      .description = "dumps the abstract syntax tree to stdout",
      .required_arg = ARG_NONE,
      .type = OPT_BOOLEAN,
      .long_flag = true,
  };
  struct Option ir_dump_flag = {
      .flag = "dump-ir",
      .description = "dumps the intermediate representation to stdout",
      .required_arg = ARG_NONE,
      .type = OPT_BOOLEAN,
      .long_flag = true,
  };
  struct Option reg_dump_flag = {
      .flag = "dump-reg",
      .description = "dumps the registers to stdout",
      .required_arg = ARG_NONE,
      .type = OPT_BOOLEAN,
      .long_flag = true,
  };

  struct Option *opts[] = {&help, &version, &ast_dump_flag, &ir_dump_flag,
                           &reg_dump_flag};
  size_t number_opts = sizeof(opts)/sizeof(struct Option *);
  char *in_filename = NULL;

  parse_args(argc, argv, opts, number_opts, &in_filename);

  if (!ir_dump_flag.out.enabled && reg_dump_flag.out.enabled) {
    log_err_final("cannot print registers without printing the IR");
  }

  if (help.out.enabled) {
    print_help(argv[0], "Take two on compiler for beans.", opts, number_opts);
    exit(EXIT_SUCCESS);
  }
  if (version.out.enabled) {
    printf("bcc2 : v0.1\n");
    exit(EXIT_SUCCESS);
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

  const uint8_t *in_file =
      mmap(NULL, in_size, PROT_READ, MAP_PRIVATE, in_fd, 0);
  if (in_file == MAP_FAILED) {
    log_err_final("unable to get contents of '%s'", in_filename);
  }

  lexer_init(in_file, in_size);

  AST ast = parse_ast(in_file);
  resolve_names(&ast);
  resolve_types(&ast);
  check_returns(&ast);

  if (ast_dump_flag.out.enabled) {
    printf("AST_DUMP:\n");
    ast_dump(stdout, &ast);
    printf("\n");
  }

  SSA_Prog ssa_prog;
  translate_ast(&ast, &ssa_prog, &ast.pool);

  if (ir_dump_flag.out.enabled) {
    printf("IR_DUMP:\n");
    ssa_prog_dump(stdout, &ssa_prog, reg_dump_flag.out.enabled);
  }

  ast_deinit(&ast);

  munmap((uint8_t *)in_file, in_size);
  return EXIT_SUCCESS;
}
