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
#include "platforms.h"

struct {
  int ast_dump;
  int ir_dump;
  int reg_dump;
  int help;
  int version;
  int list_platforms;
  const char *in_file;
  Platform *platform;
} flags;

void
parse_args(int argc, char *argv[]) {
  memset(&flags, 0, sizeof(flags));
  flags.platform = &platform_x86_64_sysv;
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

      if (strcmp(argv[i], "-platform") == 0) {
        if (argc == 2) {
          flags.list_platforms |= 1;
        } else if (i + 1 >= argc || argv[i + 1][0] == '-') {
          log_err_final("expected platform name after -platform");
        } else {
          flags.platform = NULL;
          for (size_t j = 0; platforms[j] != NULL; j++) {
            if (strcmp(platforms[j]->name, argv[i + 1]) == 0) {
              flags.platform = platforms[j];
              i++;
              break;
            }
          }
          if (flags.platform == NULL) {
            log_err_final("unknown platform name '%s'", argv[i + 1]);
          }
        }
      }
    }
  }
}

int
main(int argc, char *argv[]) {
  parse_args(argc, argv);

  if (flags.help) {
    printf("-h : prints help\n"
           "-v : prints version\n"
           "-ast : dumps ast to stdout\n"
           "-ir : dumps ir to stdout\n"
           "-regs : dumps registers to stdout\n"
           "-platform : list platforms\n"
           "-platform <platform> : selects the platform to compile for\n");
    exit(EXIT_SUCCESS);
  }

  if (flags.version) {
    printf("bcc2 : v0.1\n");
    exit(EXIT_SUCCESS);
  }

  if (flags.list_platforms) {
    printf("available platforms:\n");
    for (size_t i = 0; platforms[i] != NULL; i++) {
      printf(" - %s\n", platforms[i]->name);
    }
    exit(EXIT_SUCCESS);
  }

  if (!flags.in_file) {
    log_err_final("no input file specified");
  }

  if (!flags.ir_dump && flags.reg_dump) {
    log_err_final("cannot print registers without printing the IR");
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
