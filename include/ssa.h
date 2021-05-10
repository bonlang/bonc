#ifndef SSA_H
#define SSA_H

#include <stdint.h>
#include <stdio.h>

#include "helper.h"
#include "sym_reg.h"

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
  SymReg* result;

  union {
    struct {
      SymReg* op1;
      SymReg* op2;
    } binop;
    SymReg* copy;
    SymReg* ret;
    SourcePosition imm;
  } data;
} SSA_Inst;

typedef struct BBlock {
  /* Null if last block in function */
  int64_t id;
  Vector insts; /* Inst */
  struct BBlock* next;
} SSA_BBlock;

typedef struct {
  struct BBlock* entry;
  Vector params; /* SymReg* */
  SourcePosition name;
} SSA_Fn;

typedef struct {
  Vector fns; /* SSA_Function */
} SSA_Prog;

SSA_BBlock* bblock_init(MemPool* pool);
SSA_Inst* bblock_append(SSA_BBlock* block, MemPool* pool);
void bblock_finish(SSA_BBlock* block, SSA_BBlock* next);
void ssa_prog_dump(FILE* file, SSA_Prog* prog);

#endif
