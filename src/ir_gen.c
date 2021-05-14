#include "ir_gen.h"

#include <stdlib.h>

#define NEXT_REG(fn) (fn->cur_id++)

static int
type_sz(int ast_type) {
  switch (ast_type) {
    case TYPE_I8:
    case TYPE_U8:
      return SZ_8;
    case TYPE_I16:
    case TYPE_U16:
      printf("here\n");
      return SZ_16;
    case TYPE_I32:
    case TYPE_U32:
      return SZ_32;
    case TYPE_I64:
    case TYPE_U64:
      return SZ_64;
  }
  log_internal_err("invalid type %d", ast_type);
  return 0;
}

static RegId
new_reg(SSA_Fn *fn, int sz) {
  SSA_Reg *reg = vector_alloc(&fn->regs);
  RegId ret = (RegId)fn->regs.items; /* starts at 1 */
  reg->sz = sz;
  return ret;
}

static RegId
sym_table_reg(SSA_Fn *fn, ScopeEntry *entry) {
  if (entry->inf.id == 0) {
    return entry->inf.id = new_reg(fn, type_sz(entry->inf.type->t));
  } else {
    return entry->inf.id;
  }
}

static int
translate_binop(int typ, int binop) {
  switch (binop) {
    case BINOP_ADD:
      return INST_ADD;
    case BINOP_SUB:
      return INST_SUB;
    case BINOP_MUL:
      if (is_unsigned(typ)) {
        return INST_UMUL;
      }
      return INST_IMUL;
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

static void
inst_init(SSA_Inst *inst, int t, int sz, RegId result) {
  inst->sz = sz;
  inst->t = t;
  inst->result = result;
}

static RegId
translate_expr(Expr *expr, Scope *scope, SSA_BBlock *block, SSA_Fn *fn,
               MemPool *pool) {
  switch (expr->t) {
    case EXPR_INT:
      {
        SSA_Inst *inst = bblock_append(block);
        inst_init(inst, INST_IMM, type_sz(expr->type->t),
                  new_reg(fn, type_sz(expr->type->t)));
        inst->data.imm = expr->data.intlit.val;
        return inst->result;
      }
    case EXPR_VAR:
      {
        return sym_table_reg(fn, expr->data.var);
      }
    case EXPR_BINOP:
      {
        RegId obj1 =
            translate_expr(expr->data.binop.left, scope, block, fn, pool);
        RegId obj2 =
            translate_expr(expr->data.binop.right, scope, block, fn, pool);
        SSA_Inst *inst = bblock_append(block);
        inst->sz = type_sz(expr->type->t);
        inst->t = translate_binop(expr->type->t, expr->data.binop.op);
        inst->data.operands[0] = obj1;
        inst->data.operands[1] = obj2;
        inst->result = new_reg(fn, type_sz(expr->type->t));
        return inst->result;
      }
    case EXPR_FUNCALL:
      {
        Vector passed_params;
        vector_init(&passed_params, sizeof(RegId), pool);
        for (size_t i = 0; i < expr->data.funcall.args.items; i++) {
          Expr *temp_expr = *((Expr **)vector_idx(&expr->data.funcall.args, i));
          RegId temp_id = translate_expr(temp_expr, scope, block, fn, pool);
          vector_push(&passed_params, &temp_id);
        }
        SSA_Inst *inst = bblock_append(block);
        inst_init(inst, INST_CALLFN, type_sz(expr->type->t),
                  new_reg(fn, type_sz(expr->type->t)));
        if (expr->data.funcall.fn->inf.fn == NULL) {
          log_internal_err("cannot call runtime selected functions", NULL);
        }
        inst->data.callfn.fn = expr->data.funcall.fn->inf.fn;
        inst->data.callfn.args = passed_params;
        return inst->result;
      }
    default:
      log_internal_err("invalid expression type %d", expr->t);
      exit(EXIT_FAILURE);
  }
}

static void
translate_stmt(Stmt *stmt, Scope *scope, SSA_BBlock *block, SSA_Fn *fn,
               MemPool *pool) {
  switch (stmt->t) {
    case STMT_LET:
      if (stmt->data.let.value != NULL) {
        RegId obj =
            translate_expr(stmt->data.let.value, scope, block, fn, pool);
        SSA_Inst *inst = bblock_append(block);

        inst_init(inst, INST_COPY, type_sz(stmt->data.let.value->type->t),
                  sym_table_reg(fn, stmt->data.let.var));
        inst->data.operands[0] = obj;
      }
      break;
    case STMT_EXPR:
      {
        RegId op = translate_expr(stmt->data.expr, scope, block, fn, pool);
        SSA_Inst *inst = bblock_append(block);
        inst_init(inst, INST_COPY, type_sz(stmt->data.expr->type->t), 0);
        inst->data.operands[0] = op;
        break;
      }
    case STMT_RETURN:
      {
        RegId op = 0;
        if (stmt->data.ret != NULL) {
          op = translate_expr(stmt->data.ret, scope, block, fn, pool);
        }
        SSA_Inst *inst = bblock_append(block);
        inst_init(inst, INST_RET, 0,
                  stmt->data.let.value == NULL
                      ? SZ_NONE
                      : type_sz(stmt->data.ret->type->t));
        inst->data.operands[0] = op;
        break;
      }
    default:
      log_internal_err("invalid statement type %d", stmt->t);
  }
}

void
translate_function(Function *fn, SSA_Fn *sem_fn, MemPool *pool) {
  SSA_BBlock *block = bblock_init(pool);
  vector_init(&sem_fn->params, sizeof(RegId), pool);
  vector_init(&sem_fn->regs, sizeof(SSA_Reg), pool);
  for (size_t i = 0; i < fn->params.items; i++) {
    Param *param = vector_idx(&fn->params, i);
    RegId temp = sym_table_reg(sem_fn, param->entry);
    vector_push(&sem_fn->params, &temp);
  }

  for (size_t i = 0; i < fn->body.stmts.items; i++) {
    translate_stmt(vector_idx(&fn->body.stmts, i), fn->scope, block, sem_fn,
                   pool);
  }
  sem_fn->name = fn->name;
  sem_fn->entry = block;
}

void
translate_ast(AST *ast, SSA_Prog *prog) {
  mempool_init(&prog->pool);
  vector_init(&prog->fns, sizeof(SSA_Fn), &prog->pool);

  for (size_t i = 0; i < ast->fns.items; i++) {
    Function *fn = vector_idx(&ast->fns, i);
    SSA_Fn *ssa_fn = vector_alloc(&prog->fns);
    fn->entry->inf.fn = ssa_fn;
  }
  for (size_t i = 0; i < ast->fns.items; i++) {
    translate_function(vector_idx(&ast->fns, i), vector_idx(&prog->fns, i),
                       &prog->pool);
  }
}
