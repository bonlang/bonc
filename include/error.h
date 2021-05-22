#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdbool.h>
#include <diags.h>

bool errors_exist();
void errors_init(MemPool *pool, const uint8_t *base, const char *filename);

/* returns if no errors, otherwise prints them to file and exits with a
 * non-zero error code */
void errors_output(FILE *file);
void errors_log(Diag diag);

#endif
