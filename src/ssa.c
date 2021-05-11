#include "ssa.h"

#include <inttypes.h>

SSA_BBlock* bblock_init(MemPool* pool) {
  SSA_BBlock* block = mempool_alloc(pool, sizeof(SSA_BBlock));
  vector_init(&block->insts, sizeof(SSA_Inst), pool);
  block->next = NULL;
  return block;
}

SSA_Inst* bblock_append(SSA_BBlock* block, MemPool* pool) {
  return vector_alloc(&block->insts, pool);
}

void bblock_finalize(SSA_BBlock* block, SSA_BBlock* next) {
  block->next = next;
}

const char* binop_name_tbl[] = {"add", "sub", "mul", "idiv", "udiv"};
const char* sz_name_tbl[] = {"", "8", "16", "32", "64"};

static void dump_nullable_reg(FILE* file, RegId reg) {
  if (reg == 0) {
    fprintf(file, "_");
  } else {
    fprintf(file, "%%%ld", reg);
  }
}

static void inst_dump(FILE* file, SSA_Inst* inst) {
  switch (inst->t) {
    case INST_COPY:
      dump_nullable_reg(file, inst->result);
      fprintf(file, " =%s copy %%%ld", sz_name_tbl[inst->sz], inst->result);
      break;
    case INST_IMM:
      dump_nullable_reg(file, inst->result);
      fprintf(file, " =%s $%.*s", sz_name_tbl[inst->sz], (int)inst->data.imm.sz,
              (char*)inst->data.imm.start);
      break;
    case INST_ADD:
    case INST_SUB:
    case INST_MUL:
    case INST_IDIV:
    case INST_UDIV:
      dump_nullable_reg(file, inst->result);
      fprintf(file, " =%s %s %%%ld %%%ld", sz_name_tbl[inst->sz],
              binop_name_tbl[inst->t - INST_ADD], inst->data.binop.op1,
              inst->data.binop.op2);
      break;
    case INST_RET:
      fprintf(file, "ret ");
      dump_nullable_reg(file, inst->data.ret);
      break;
    default:
      log_internal_err("cannot print instruction: %d", inst->t);
  }
  fprintf(file, "\n");
}

void bblock_dump(FILE* file, SSA_BBlock* block) {
  for (size_t i = 0; i < block->insts.items; i++) {
    SSA_Inst* inst = vector_idx(&block->insts, i);
    inst_dump(file, inst);
  }
}

void function_dump(FILE* file, SSA_Fn* fn, int reg_dump) {
  fprintf(file, "fn %.*s(", (int)fn->name.sz, (char*)fn->name.start);

  if (fn->params.items > 0) {
    for (size_t i = 0; i < fn->params.items - 1; i++) {
      RegId param = *((RegId*)vector_idx(&fn->params, i));
      SSA_Reg* reg = vector_idx(&fn->regs, param - 1);
      fprintf(file, "%%%ld: %s, ", param, sz_name_tbl[reg->sz]);
    }
    RegId param = *((RegId*)vector_idx(&fn->params, fn->params.items - 1));
    SSA_Reg* reg = vector_idx(&fn->regs, param - 1);
    fprintf(file, "%%%ld: %s)\n", param, sz_name_tbl[reg->sz]);
  } else {
    fprintf(file, ")\n");
  }

  bblock_dump(file, fn->entry);

  if (reg_dump) {
    fprintf(file, "\n");
    for (size_t i = 0; i < fn->regs.items; i++) {
      SSA_Reg* reg = vector_idx(&fn->regs, i);
      fprintf(file, "| %zd | bit%s |\n", i + 1, sz_name_tbl[reg->sz]);
    }
  }
  fprintf(file, "\n");
}

void ssa_prog_dump(FILE* file, SSA_Prog* prog, int reg_dump) {
  for (size_t i = 0; i < prog->fns.items; i++) {
    function_dump(file, vector_idx(&prog->fns, i), reg_dump);
  }
}
