#include "ir_gen.h"

#include <stdlib.h>

static int type_sz(int ast_type) {
  switch (ast_type) {
    case TYPE_I8:
    case TYPE_U8:
      return SZ_BYTE;
    case TYPE_I16:
    case TYPE_U16:
      return SZ_HWORD;
    case TYPE_I32:
    case TYPE_U32:
      return SZ_WORD;
    case TYPE_I64:
    case TYPE_U64:
      return SZ_QWORD;
  }
  log_internal_err("invalid type %d", ast_type);
  return 0;
}

static int get_sym_table_reg(ScopeEntry* entry) {
  if (entry->inf.id == 0) {
    return entry->inf.id = next_reg();
  }
  return entry->inf.id;
}

static int translate_binop(int typ, int binop) {
  switch (binop) {
    case BINOP_ADD:
      return INST_ADD;
    case BINOP_SUB:
      return INST_SUB;
    case BINOP_MUL:
      return INST_MUL;
    case BINOP_DIV:
      if (is_unsigned(typ)) {
        return INST_UDIV;
      }
      return INST_IDIV;
    default:
      log_internal_err("cannot translate binop %d", binop);
      return 0;
  }
}
static SSA_Obj translate_expr(Expr* expr, Scope* scope, SSA_BBlock* block,
                              MemPool* pool) {
  (void)scope;
  (void)block;
  (void)pool;
  SSA_Obj ret;
  switch (expr->t) {
    case EXPR_INT:
      ret.t = OBJ_INT;
      ret.data.intnum = expr->pos;
      return ret;
    case EXPR_VAR:
      ret.t = OBJ_REG;
      ret.data.reg = get_sym_table_reg(expr->data.var);
      return ret;
    case EXPR_BINOP: {
      SSA_Obj obj1 = translate_expr(expr->data.binop.left, scope, block, pool);
      SSA_Obj obj2 = translate_expr(expr->data.binop.right, scope, block, pool);
      SSA_Inst* inst = bblock_append(block, pool);
      inst->sz = type_sz(expr->type->t);
      inst->t = translate_binop(expr->type->t, expr->data.binop.op);
      inst->data.binop.op1 = obj1;
      inst->data.binop.op2 = obj2;
      inst->result.t = OBJ_REG;
      inst->result.data.reg = next_reg();
      return inst->result;
    }
    default:
      log_internal_err("cannot translate expression into SSA form ");
      exit(EXIT_FAILURE);
  }
}

static void translate_stmt(Stmt* stmt, Scope* scope, SSA_BBlock* block,
                           MemPool* pool) {
  switch (stmt->t) {
    case STMT_LET:
      if (stmt->data.let.value != NULL) {
        SSA_Obj obj = translate_expr(stmt->data.let.value, scope, block, pool);
        SSA_Inst* inst = bblock_append(block, pool);
        inst->sz = type_sz(stmt->data.let.value->type->t);
        inst->t = INST_COPY;
        inst->data.copy = obj;
        inst->result.t = OBJ_REG;
        inst->result.data.reg = get_sym_table_reg(stmt->data.let.var);
      }
      break;
    default:
      log_internal_err("cannot translate statement into SSA form ");
  }
}

SSA_BBlock* translate_function(Function* fn, MemPool* pool) {
  SSA_BBlock* block = bblock_init(pool);
  for (size_t i = 0; i < fn->body.stmts.items; i++) {
    translate_stmt(vector_idx(&fn->body.stmts, i), fn->scope, block, pool);
  }
  return block;
}
