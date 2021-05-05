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

static void obj_dump(FILE* file, SSA_Obj* obj) {
  switch (obj->t) {
    case OBJ_NONE:
      fprintf(file, "_");
      break;
    case OBJ_INT:
      fprintf(file, "$%.*s", (int)obj->data.intnum.sz,
              (char*)obj->data.intnum.start);
      break;
    case OBJ_REG:
      fprintf(file, "%%%" PRId64, obj->data.reg);
      break;
  }
}

const char* binop_name_tbl[] = {"add", "sub", "mul", "idiv", "udiv"};
const char sz_name_tbl[] = {' ', 'b', 'h', 'w', 'q'};

static void inst_dump(FILE* file, SSA_Inst* inst) {
  switch (inst->t) {
    case INST_COPY:
      obj_dump(file, &inst->result);
      fprintf(file, " =%c copy ", sz_name_tbl[inst->sz]);
      obj_dump(file, &inst->data.copy);
      break;
    case INST_ADD:
    case INST_SUB:
    case INST_MUL:
    case INST_IDIV:
    case INST_UDIV:
      obj_dump(file, &inst->result);
      fprintf(file, " =%c %s ", sz_name_tbl[inst->sz],
              binop_name_tbl[inst->t - INST_ADD]);
      obj_dump(file, &inst->data.binop.op1);
      fprintf(file, " ");
      obj_dump(file, &inst->data.binop.op2);
      break;
    case INST_RET:
      fprintf(file, "ret ");
      obj_dump(file, &inst->data.ret);
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
