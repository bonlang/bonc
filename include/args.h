#ifndef ARGS_H
#define ARGS_H
#include <stddef.h>
#include <stdbool.h>

struct Option {
  const char *flag;
  const char *description;
  const char *argument_name;
  bool long_flag;
  
  bool enabled;
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
    const char *string;
    int integer;
  } out;
};

void parse_args(int argc, char *argv[], struct Option *opts[], size_t opts_size, char **input_file);
void print_flags(struct Option *opts[], size_t opts_size);

#endif
