#include <error.h>
#include <stdlib.h>

#include "diags.txt"
Vector errs; /* Diag */
const uint8_t *base;

void
errors_init(MemPool *pool, const uint8_t *_base) {
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
    (void)diag;
    diag_output(diag, file);
  }
  fprintf(file, "%ld error%s found. Aborting.\n", errs.items,
          errs.items == 1 ? "" : "s");
  exit(EXIT_FAILURE);
}

void
errors_log(Diag diag) {
  vector_push(&errs, &diag);
}
