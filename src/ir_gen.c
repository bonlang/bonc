#include "ir_gen.h"

#include <stdlib.h>

#define NEXT_REG(fn) (fn->cur_id++)

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

static SymReg* new_reg(int sz, MemPool* pool) {
  SymReg* ret = mempool_alloc(pool, sizeof(SymReg));
  ret->sz = sz;
  ret->vn = next_vn();
  return ret;
}

static SymReg* get_sym_table_reg(ScopeEntry* entry, MemPool* pool) {
  if (entry->reg == NULL) {
    return entry->reg = new_reg(type_sz(entry->inf.type->t), pool);
  } else {
    return entry->reg;
  }
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

static SymReg* translate_expr(Expr* expr, Scope* scope, SSA_BBlock* block,
                              MemPool* pool) {
  (void)scope;
  (void)block;
  (void)pool;
  switch (expr->t) {
    case EXPR_INT: {
      SSA_Inst* inst = bblock_append(block, pool);
      inst->t = INST_IMM;
      inst->data.imm = expr->data.intlit.literal;
      return inst->result = new_reg(expr->type->t, pool);
    }
    case EXPR_VAR: {
      return get_sym_table_reg(expr->data.var, pool);
    }
    case EXPR_BINOP: {
      SymReg* obj1 = translate_expr(expr->data.binop.left, scope, block, pool);
      SymReg* obj2 = translate_expr(expr->data.binop.right, scope, block, pool);
      SSA_Inst* inst = bblock_append(block, pool);
      inst->sz = type_sz(expr->type->t);
      inst->t = translate_binop(expr->type->t, expr->data.binop.op);
      inst->data.binop.op1 = obj1;
      inst->data.binop.op2 = obj2;
      inst->result = new_reg(type_sz(expr->type->t), pool);
      return inst->result;
    }
    default:
      log_internal_err("invalid expression type %d", expr->t);
      exit(EXIT_FAILURE);
  }
}

static void inst_init(SSA_Inst* inst, int t, int sz, SymReg* result) {
  inst->sz = sz;
  inst->t = t;
  inst->result = result;
}

static void translate_stmt(Stmt* stmt, Scope* scope, SSA_BBlock* block,
                           MemPool* pool) {
  switch (stmt->t) {
    case STMT_LET:
      if (stmt->data.let.value != NULL) {
        SymReg* obj = translate_expr(stmt->data.let.value, scope, block, pool);
        SSA_Inst* inst = bblock_append(block, pool);

        inst_init(inst, INST_COPY, type_sz(stmt->data.let.value->type->t),
                  get_sym_table_reg(stmt->data.let.var, pool));
        inst->data.copy = obj;
      }
      break;
    case STMT_EXPR: {
      SymReg* op = translate_expr(stmt->data.expr, scope, block, pool);
      SSA_Inst* inst = bblock_append(block, pool);
      inst_init(inst, INST_COPY, type_sz(stmt->data.expr->type->t), NULL);
      inst->data.copy = op;
      break;
    }
    case STMT_RETURN: {
      SymReg* op = NULL;
      if (stmt->data.ret != NULL) {
        op = translate_expr(stmt->data.ret, scope, block, pool);
      }
      SSA_Inst* inst = bblock_append(block, pool);
      inst->sz = stmt->data.let.value == NULL
                     ? SZ_NONE
                     : type_sz(stmt->data.ret->type->t);
      inst->t = INST_RET;
      inst->data.ret = op;
      inst->result = NULL;
      break;
    }
    default:
      log_internal_err("invalid statement type %d", stmt->t);
  }
}

void translate_function(Function* fn, SSA_Fn* sem_fn, MemPool* pool) {
  SSA_BBlock* block = bblock_init(pool);
  vector_init(&sem_fn->params, sizeof(SymReg*), pool);
  for (size_t i = 0; i < fn->params.items; i++) {
    Param* param = vector_idx(&fn->params, i);
    SymReg* temp = get_sym_table_reg(param->entry, pool);
    vector_push(&sem_fn->params, &temp, pool);
  }

  for (size_t i = 0; i < fn->body.stmts.items; i++) {
    translate_stmt(vector_idx(&fn->body.stmts, i), fn->scope, block, pool);
  }
  sem_fn->name = fn->name;
  sem_fn->entry = block;
}

void translate_ast(AST* ast, SSA_Prog* prog, MemPool* pool) {
  vector_init(&prog->fns, sizeof(SSA_Fn), pool);

  for (size_t i = 0; i < ast->fns.items; i++) {
    translate_function(vector_idx(&ast->fns, i), vector_alloc(&prog->fns, pool),
                       pool);
  }
}
