#include <error.h>
#include <stdlib.h>

#define KRED "\x1B[31m"
#define KNRM "\x1B[0m"

static void
error_output_type(FILE *file, Type *type) {
  (void)type;
  fprintf(file, "TYPE_SPEC");
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
