#include "ssa.h"

#include <inttypes.h>

const uint8_t inst_arity_tbl[] = {
    [INST_ADD] = 2,  [INST_SUB] = 2,  [INST_MUL] = 2, [INST_IDIV] = 2,
    [INST_UDIV] = 2, [INST_COPY] = 1, [INST_RET] = 1, [INST_IMM] = 1,
};

const uint8_t inst_returns_tbl[] = {
    [INST_ADD] = 1,  [INST_SUB] = 1,  [INST_MUL] = 1, [INST_IDIV] = 1,
    [INST_UDIV] = 1, [INST_COPY] = 1, [INST_RET] = 0, [INST_IMM] = 1,
};

const char *inst_name_tbl[] = {
    [INST_ADD] = "add",   [INST_SUB] = "sub",   [INST_MUL] = "mul",
    [INST_IDIV] = "idiv", [INST_UDIV] = "udiv", [INST_COPY] = "copy",
    [INST_RET] = "ret",   [INST_IMM] = "imm",
};

SSA_BBlock *
bblock_init(MemPool *pool) {
  SSA_BBlock *block = mempool_alloc(pool, sizeof(SSA_BBlock));
  vector_init(&block->insts, sizeof(SSA_Inst), pool);
  block->next = NULL;
  return block;
}

SSA_Inst *
bblock_append(SSA_BBlock *block, MemPool *pool) {
  return vector_alloc(&block->insts, pool);
}

void
bblock_finalize(SSA_BBlock *block, SSA_BBlock *next) {
  block->next = next;
}

static const char *sz_name_tbl[] = {"", "8", "16", "32", "64"};

static void
dump_nullable_reg(FILE *file, RegId reg) {
  if (reg == 0) {
    fprintf(file, "_");
  } else {
    fprintf(file, "%%%ld", reg);
  }
}

static void
inst_dump(FILE *file, SSA_Inst *inst) {
  if (inst->t != INST_IMM) {
    dump_nullable_reg(file, inst->result);
    fprintf(file, " =%s %s", sz_name_tbl[inst->sz], inst_name_tbl[inst->t]);

    for (int i = 0; i < inst_arity_tbl[inst->t]; i++) {
      fprintf(file, " %%%" PRIu64, inst->data.operands[i]);
    }
  } else {
    dump_nullable_reg(file, inst->result);
    fprintf(file, " =%s $%.*s", sz_name_tbl[inst->sz], (int)inst->data.imm.sz,
            (char *)inst->data.imm.start);
  }

  fprintf(file, "\n");
}

void
bblock_dump(FILE *file, SSA_BBlock *block) {
  for (size_t i = 0; i < block->insts.items; i++) {
    SSA_Inst *inst = vector_idx(&block->insts, i);
    inst_dump(file, inst);
  }
}

void
function_dump(FILE *file, SSA_Fn *fn, int reg_dump) {
  fprintf(file, "fn %.*s(", (int)fn->name.sz, (char *)fn->name.start);

  if (fn->params.items > 0) {
    for (size_t i = 0; i < fn->params.items - 1; i++) {
      RegId param = *((RegId *)vector_idx(&fn->params, i));
      SSA_Reg *reg = vector_idx(&fn->regs, param - 1);
      fprintf(file, "%%%ld: %s, ", param, sz_name_tbl[reg->sz]);
    }
    RegId param = *((RegId *)vector_idx(&fn->params, fn->params.items - 1));
    SSA_Reg *reg = vector_idx(&fn->regs, param - 1);
    fprintf(file, "%%%ld: %s)\n", param, sz_name_tbl[reg->sz]);
  } else {
    fprintf(file, ")\n");
  }

  bblock_dump(file, fn->entry);

  if (reg_dump) {
    fprintf(file, "\n");
    for (size_t i = 0; i < fn->regs.items; i++) {
      SSA_Reg *reg = vector_idx(&fn->regs, i);
      fprintf(file, "| %zd | bit%s |\n", i + 1, sz_name_tbl[reg->sz]);
    }
  }
  fprintf(file, "\n");
}

void
ssa_prog_dump(FILE *file, SSA_Prog *prog, int reg_dump) {
  for (size_t i = 0; i < prog->fns.items; i++) {
    function_dump(file, vector_idx(&prog->fns, i), reg_dump);
  }
}
