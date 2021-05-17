#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
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

struct Option {
  const char *flag;
  const char *description;
  bool long_flag;

  enum {
    ARG_NONE,
    ARG_REQUIRED,
    ARG_OPTIONAL,
  } required_arg;

  enum {
    OPT_BOOLEAN,
    OPT_STRING,
    OPT_INT,
  } type;

  union {
    bool enabled;
    const char *string;
    int integer;
  } out;
};

bool
fill_flag_argument(const char *value, struct Option *opt) {
  bool argument_used = false;
  if (opt->required_arg == ARG_NONE && value != NULL) {
    log_err_final("option '%s' doesn't allow an argument", opt->flag);
  } else if (opt->required_arg == ARG_REQUIRED && value == NULL) {
    log_err_final("option '%s' requires an argument", opt->flag);
  }
  switch (opt->type) {
    case OPT_BOOLEAN:
      opt->out.enabled = true;
      break;
    case OPT_STRING:
      opt->out.string = value;
      argument_used = true;
      break;
    case OPT_INT:
      opt->out.integer = atoi(value);
      argument_used = true;
      break;
  }
  return argument_used;
}

void
long_flag_parse(const char *flag, struct Option *opts[], size_t opts_size) {
  for (size_t i = 0; i < opts_size; i++) {
    if (opts[i]->long_flag &&
        !strncmp(flag, opts[i]->flag, strlen(opts[i]->flag))) {
      char *value;
      if ((value = strchr(flag, '=')) != NULL) {
        value++;
      }
      fill_flag_argument(value, opts[i]);
      return;
    }
  }
  log_err_final("unrecognized option %s", flag);
}

int
short_flag_parse(const char *flag, char *next_argv, struct Option *opts[],
                 size_t opts_size) {
  for (size_t i = 0; i < opts_size; i++) {
    if (!opts[i]->long_flag &&
        !strncmp(flag, opts[i]->flag, strlen(opts[i]->flag))) {
      char *value = next_argv;
      return fill_flag_argument(value, opts[i]) + 1;
    }
  }
  log_err_final("unrecognized option %s", flag);
  return 0; // unreachable, just to stop a warning
}

void
parse_args(int argc, char *argv[], struct Option *opts[], size_t opts_size,
           char **input_file) {
  for (int i = 1; i < argc;) {
    if (argv[i][0] == '-') {
      errno = 0;
      if (argv[i][1] == '-') {
        long_flag_parse(&argv[i][2], opts, opts_size);
        i++;
      } else {
        i += short_flag_parse(&argv[i][1], argc - i != 1 ? argv[i + 1] : NULL,
                              opts, opts_size);
      }
    } else {
      if (*input_file != NULL) {
        log_err_final("can't specify more than one input file");
      }
      *input_file = argv[i];
      i++;
    }
  }
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
  char *in_filename = NULL;

  parse_args(argc, argv, opts, sizeof(opts), &in_filename);

  if (!ir_dump_flag.out.enabled && reg_dump_flag.out.enabled) {
    log_err_final("cannot print registers without printing the IR");
  }

  if (help.out.enabled) {
    printf("-h : prints help\n-v : prints version\n-ast : dumps ast to "
           "stdout\n-ir : dumps ir to stdout\n-regs : dumps registers to "
           "stdout\n");
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
