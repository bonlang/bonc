#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "helper.h"
#include "args.h"

static bool
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

static void
long_flag_parse(const char *flag, struct Option *opts[], size_t opts_length) {
  for (size_t i = 0; i < opts_length; i++) {
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
short_flag_parse(const char *flag, char *next_argv, struct Option *opts[],
                 size_t opts_length) {
  for (size_t i = 0; i < opts_length; i++) {
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
parse_args(int argc, char *argv[], struct Option *opts[], size_t opts_length,
           char **input_file) {
  for (int i = 1; i < argc;) {
    if (argv[i][0] == '-') {
      if (argv[i][1] == '-') {
        long_flag_parse(&argv[i][2], opts, opts_length);
        i++;
      } else {
        i += short_flag_parse(&argv[i][1], argc - i != 1 ? argv[i + 1] : NULL,
                              opts, opts_length);
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
print_flags(struct Option *opts[], size_t opts_length) {
  for(size_t i = 0; i < opts_length; i++) {
    printf("%s%s", opts[i]->long_flag ? "--": "-", opts[i]->flag);
    if(opts[i]->required_arg != ARG_NONE) {
      printf("%c%s", opts[i]->long_flag ? '=': ' ', opts[i]->argument_name);
    }
    printf("\t%s\n", opts[i]->description);
  }
}
