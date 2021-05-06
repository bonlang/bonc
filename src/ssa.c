#include "ssa.h"

#include <inttypes.h>

SSA_BBlock* bblock_init(MemPool* pool) {
  static int64_t id_cnt = 0;
  SSA_BBlock* block = mempool_alloc(pool, sizeof(SSA_BBlock));
  block->id = id_cnt++;
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

static void reg_dump(FILE* file, SymReg* reg) {
  if (reg == NULL) {
    fprintf(file, "_");
  } else {
    fprintf(file, "%%%" PRId64, reg->vn);
  }
}

const char* binop_name_tbl[] = {"add", "sub", "mul", "idiv", "udiv"};
const char sz_name_tbl[] = {' ', 'b', 'h', 'w', 'q'};

static void inst_dump(FILE* file, SSA_Inst* inst) {
  switch (inst->t) {
    case INST_COPY:
      reg_dump(file, inst->result);
      fprintf(file, " =%c copy ", sz_name_tbl[inst->sz]);
      reg_dump(file, inst->data.copy);
      break;
    case INST_IMM:
      reg_dump(file, inst->result);
      fprintf(file, " =%c $%.*s", sz_name_tbl[inst->result->sz],
              (int)inst->data.imm.sz, (char*)inst->data.imm.start);
      break;
    case INST_ADD:
    case INST_SUB:
    case INST_MUL:
    case INST_IDIV:
    case INST_UDIV:
      reg_dump(file, inst->result);
      fprintf(file, " =%c %s ", sz_name_tbl[inst->sz],
              binop_name_tbl[inst->t - INST_ADD]);
      reg_dump(file, inst->data.binop.op1);
      fprintf(file, " ");
      reg_dump(file, inst->data.binop.op2);
      break;
    case INST_RET:
      fprintf(file, "ret ");
      reg_dump(file, inst->data.ret);
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

void function_dump(FILE* file, SSA_Fn* fn) {
  fprintf(file, "fn %.*s\n", (int)fn->name.sz, (char*)fn->name.start);
  bblock_dump(file, fn->entry);
  fprintf(file, "\n");
}

void ssa_prog_dump(FILE* file, SSA_Prog* prog) {
  for (size_t i = 0; i < prog->fns.items; i++) {
    function_dump(file, vector_idx(&prog->fns, i));
  }
}
