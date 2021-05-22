#include <error.h>
#include <stdlib.h>

#define KRED "\x1B[31m"
#define KNRM "\x1B[0m"

const char *pp_typename_table[] = {
    [TYPE_U8] = "u8",     [TYPE_I8] = "i8",   [TYPE_U16] = "u16",
    [TYPE_I16] = "i16",   [TYPE_U32] = "u32", [TYPE_I32] = "i32",
    [TYPE_U64] = "u64",   [TYPE_I64] = "i64", [TYPE_BOOL] = "bool",
    [TYPE_VOID] = "void", [TYPE_FN] = "fn"};

static void
error_output_type(FILE *file, Type *type) {
  switch (type->t) {
    case TYPE_FN:
      {
        fprintf(file, "(");
        if (type->data.fn.args.items != 0) {
          for (size_t i = 0; i < type->data.fn.args.items - 1; i++) {
            Type **arg_type = vector_idx(&type->data.fn.args, i);
            error_output_type(file, *arg_type);
            fprintf(file, ", ");
          }
          Type **arg_type =
              vector_idx(&type->data.fn.args, type->data.fn.args.items - 1);
          error_output_type(file, *arg_type);
        }
        fprintf(file, ") -> ");
        error_output_type(file, type->data.fn.ret);
      }
      break;
    default:
      fprintf(file, "%s", pp_typename_table[type->t]);
  }
}
static void
error_output_pos(FILE *file, SourcePosition pos) {
  fprintf(file, "%.*s", (int)pos.sz, (char *)pos.start);
}
static void
error_output_char(FILE *file, uint8_t c) {
  fprintf(file, "%c", c);
}
static void
error_output_int(FILE *file, int i) {
  fprintf(file, "%d", i);
}

Vector errs; /* Diag */
const uint8_t *base;

#include "diags.txt"

void
errors_init(MemPool *pool, const uint8_t *_base) {
  (void)error_output_type;
  (void)error_output_char;
  (void)error_output_pos;
  (void)error_output_int;
  vector_init(&errs, sizeof(Diag), pool);
  base = _base;
}

bool
errors_exist() {
  return errs.items != 0;
}

void
errors_output(FILE *file) {
  if (errs.items == 0) {
    return;
  }
  for (size_t i = 0; i < errs.items; i++) {
    Diag *diag = vector_idx(&errs, i);
    fprintf(file, KRED "error" KNRM ": ");
    diag_output(diag, file);
    fprintf(file, ".\n");
  }
  fprintf(file, "%ld error%s found. Aborting.\n", errs.items,
          errs.items == 1 ? "" : "s");
  exit(EXIT_FAILURE);
}

void
errors_log(Diag diag) {
  vector_push(&errs, &diag);
  /* this is 100% temporary and will be removed when the error handling system
   * is designed to handle multiple errors */
  errors_output(stdout);
}
