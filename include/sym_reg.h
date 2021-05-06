#ifndef SYM_REG_H
#define SYM_REG_H

#include <stdint.h>

enum {
  SZ_NONE,
  SZ_BYTE,
  SZ_HWORD,
  SZ_WORD,
  SZ_QWORD,
};

typedef struct {
  int sz;
  int64_t vn;
} SymReg;

#endif
