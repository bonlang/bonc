#ifndef SSA_H
#define SSA_H

#include <stdint.h>
#include <stdio.h>

#include "helper.h"

typedef uint64_t RegId;

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
  INST_IMUL,
  INST_UMUL,
  INST_IDIV,
  INST_UDIV,
  INST_COPY,
  INST_RET,
  INST_IMM,
  INST_CALLFN,
};

/* Store the number of *register* arguments an instruction takes */
extern const uint8_t inst_arity_tbl[];
extern const uint8_t inst_returns_tbl[];
extern const char *inst_name_tbl[];

typedef struct SSA_Fn SSA_Fn;

typedef struct {
  int t;
  int sz;
  RegId result;

  union {
    RegId operands[2];
    struct {
      struct SSA_Fn *fn;
      Vector args; /* RegId */
    } callfn;
    uint64_t imm;
  } data;
} SSA_Inst;

typedef struct BBlock {
  Vector insts; /* Inst */
  /* Null if last block in function */
  struct BBlock *next;
} SSA_BBlock;

typedef struct {
  int sz;
  /* more stuff */
} SSA_Reg;

struct SSA_Fn {
  SSA_BBlock *entry;
  Vector params; /* RegId */
  Vector regs;   /* SSA_Reg */
  SourcePosition name;
};

typedef struct {
  MemPool pool;
  Vector fns; /* SSA_Function */
} SSA_Prog;

SSA_BBlock *bblock_init(MemPool *pool);
SSA_Inst *bblock_append(SSA_BBlock *block);

void bblock_insert_inst(SSA_BBlock *block, size_t idx, SSA_Inst *inst);
void bblock_remove_inst(SSA_BBlock *block, size_t idx);
void bblock_replace_reg(SSA_BBlock *block, RegId find, RegId replace,
                        size_t start, size_t end);

RegId ssa_new_reg(SSA_Fn *fn, int sz);

void ssa_prog_dump(FILE *file, SSA_Prog *prog, int reg_dump);

#endif
