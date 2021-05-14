#include "ssa.h"

#include <inttypes.h>

const int inst_arity_tbl[] = {
    [INST_ADD] = 2,  [INST_SUB] = 2,     [INST_IMUL] = 2, [INST_UMUL] = 2,
    [INST_IDIV] = 2, [INST_UDIV] = 2,    [INST_COPY] = 1, [INST_RET] = 1,
    [INST_IMM] = 1,  [INST_CALLFN] = -1,
};

const uint8_t inst_returns_tbl[] = {
    [INST_ADD] = 1,  [INST_SUB] = 1,    [INST_UMUL] = 1, [INST_IMUL] = 1,
    [INST_IDIV] = 1, [INST_UDIV] = 1,   [INST_COPY] = 1, [INST_RET] = 0,
    [INST_IMM] = 1,  [INST_CALLFN] = 1,
};

const char *inst_name_tbl[] = {
    [INST_ADD] = "add",       [INST_SUB] = "sub",   [INST_IMUL] = "imul",
    [INST_UMUL] = "umul",     [INST_IDIV] = "idiv", [INST_UDIV] = "udiv",
    [INST_COPY] = "copy",     [INST_RET] = "ret",   [INST_IMM] = "imm",
    [INST_CALLFN] = "callfn",
};

SSA_BBlock *
bblock_init(MemPool *pool) {
  SSA_BBlock *block = mempool_alloc(pool, sizeof(SSA_BBlock));
  vector_init(&block->insts, sizeof(SSA_Inst), pool);
  block->next = NULL;
  return block;
}

SSA_Inst *
bblock_append(SSA_BBlock *block) {
  return vector_alloc(&block->insts);
}

void
bblock_finalize(SSA_BBlock *block, SSA_BBlock *next) {
  block->next = next;
}

static const char *sz_name_tbl[] = {"", "8", "16", "32", "64"};

static void
dump_nullable_reg(FILE *file, RegId reg, int sz) {
  if (reg != 0) {
    fprintf(file, "%%%ld =%s ", reg, sz_name_tbl[sz]);
  }
}

static void
inst_dump(FILE *file, SSA_Inst *inst) {
  if (inst->t == INST_IMM) {
    dump_nullable_reg(file, inst->result, inst->sz);
    fprintf(file, "$%" PRIu64, inst->data.imm);
  } else if (inst->t == INST_CALLFN) {
    dump_nullable_reg(file, inst->result, inst->sz);
    fprintf(file, "callfn %.*s(", (int)inst->data.callfn.fn->name.sz,
            (char *)inst->data.callfn.fn->name.start);
    if (inst->data.callfn.args.items != 0) {
      for (size_t i = 0; i < inst->data.callfn.args.items - 1; i++) {
        fprintf(file, "%%%zd, ",
                *((RegId *)vector_idx(&inst->data.callfn.args, i)));
      }
      fprintf(file, "%%%zd",
              *((RegId *)vector_idx(&inst->data.callfn.args,
                                    inst->data.callfn.args.items - 1)));
    }
    fprintf(file, ")");
  } else {
    dump_nullable_reg(file, inst->result, inst->sz);
    fprintf(file, "%s", inst_name_tbl[inst->t]);

    for (int i = 0; i < inst_arity_tbl[inst->t]; i++) {
      fprintf(file, " %%%" PRIu64, inst->data.operands[i]);
    }
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
