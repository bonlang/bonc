#ifndef SSA_H
#define SSA_H

#include <stdint.h>
#include <stdio.h>

#include "helper.h"

enum {
  SZ_NONE,
  SZ_8,
  SZ_16,
  SZ_32,
  SZ_64,
};

enum {
  INST_ADD,
  INST_SUB,
  INST_MUL,
  INST_IDIV,
  INST_UDIV,
  INST_COPY,
  INST_RET,
  INST_IMM,
};

typedef struct {
  int t;
  int sz;
  uint64_t result;

  union {
    struct {
      uint64_t op1;
      uint64_t op2;
    } binop;
    uint64_t copy;
    uint64_t ret;
    SourcePosition imm;
  } data;
} SSA_Inst;

typedef struct BBlock {
  Vector insts; /* Inst */
  /* Null if last block in function */
  struct BBlock* next;
} SSA_BBlock;

typedef struct {
  int sz;
  /* more stuff */
} SSA_Reg;

typedef struct {
  SSA_BBlock* entry;
  Vector params; /* uint64_t */
  Vector regs;   /* SSA_Reg */
  SourcePosition name;
} SSA_Fn;

typedef struct {
  uint64_t next_id;
  Vector fns; /* SSA_Function */
} SSA_Prog;

SSA_BBlock* bblock_init(MemPool* pool);
SSA_Inst* bblock_append(SSA_BBlock* block, MemPool* pool);
void bblock_finish(SSA_BBlock* block, SSA_BBlock* next);
void ssa_prog_dump(FILE* file, SSA_Prog* prog);

#endif
