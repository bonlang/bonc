#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "helper.h"
#include "args.h"

#define MAX_ARG_LENGTH 30

static bool
fill_flag_argument(const char *value, struct Option *opt) {
  bool argument_used = false;
  if (opt->required_arg == ARG_NONE && value != NULL) {
    log_err_final("option '%s' doesn't allow an argument", opt->flag);
  } else if (opt->required_arg == ARG_REQUIRED && value == NULL) {
    log_err_final("option '%s' requires an argument", opt->flag);
  }
  opt->enabled = true;
  switch (opt->type) {
    case OPT_BOOLEAN:
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

static void
long_flag_parse(const char *flag, struct Option *opts[]) {
  for (size_t i = 0; opts[i] != NULL; i++) {
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

static int
short_flag_parse(const char *flag, char *next_argv, struct Option *opts[]) {
  for (size_t i = 0; opts[i] != NULL; i++) {
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
parse_args(int argc, char *argv[], struct Option *opts[], char **input_file) {
  for (int i = 1; i < argc;) {
    if (argv[i][0] == '-') {
      if (argv[i][1] == '-') {
        long_flag_parse(&argv[i][2], opts);
        i++;
      } else {
        i += short_flag_parse(&argv[i][1], argc - i != 1 ? argv[i + 1] : NULL,
                              opts);
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

void
print_flags(struct Option *opts[]) {
  printf("Flags:\n");
  for (size_t i = 0; opts[i] != NULL; i++) {
    printf("  %2s%-*s", opts[i]->long_flag ? "--" : "-",
           opts[i]->required_arg == ARG_NONE ? MAX_ARG_LENGTH : 0,
           opts[i]->flag);
    if (opts[i]->required_arg != ARG_NONE) {
      printf("%c%-*s", opts[i]->long_flag ? '=' : ' ',
             (int)(MAX_ARG_LENGTH - strlen(opts[i]->flag) - 1),
             opts[i]->argument_name);
    }
    printf("%s\n", opts[i]->description);
  }
}
