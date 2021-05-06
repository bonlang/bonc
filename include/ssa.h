#ifndef SSA_H
#define SSA_H

#include <stdint.h>
#include <stdio.h>

#include "helper.h"

enum {
  OBJ_NONE,
  OBJ_INT,
  OBJ_REG,
};

typedef struct {
  int t;
  union {
    int64_t reg;
    SourcePosition intnum;
  } data;
} SSA_Obj;

enum {
  INST_ADD,
  INST_SUB,
  INST_MUL,
  INST_IDIV,
  INST_UDIV,
  INST_COPY,
  INST_RET,
};

enum {
  SZ_NONE,
  SZ_BYTE,
  SZ_HWORD,
  SZ_WORD,
  SZ_QWORD,
};

typedef struct {
  int t;
  int sz;
  SSA_Obj result;
  union {
    struct {
      SSA_Obj op1;
      SSA_Obj op2;
    } binop;
    SSA_Obj copy;
    SSA_Obj ret;
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
} SSA_Fn;

typedef struct {
  Vector fns; /* SSA_Function */
} SSA_Prog;

SSA_BBlock* bblock_init(MemPool* pool);
SSA_Inst* bblock_append(SSA_BBlock* block, MemPool* pool);
void bblock_finish(SSA_BBlock* block, SSA_BBlock* next);
void ssa_prog_dump(FILE* file, SSA_Prog* prog);

#endif
