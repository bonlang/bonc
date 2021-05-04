#include "semantics.h"

static inline int is_integer_type(int t) {
  return t == TYPE_U8 || t == TYPE_U16 || t == TYPE_U32 || t == TYPE_U64 ||
         t == TYPE_I8 || t == TYPE_I16 || t == TYPE_I32 || t == TYPE_I64 ||
         t == TYPE_INT;
}

static Type* coerce_type(Type* left, Type* right, MemPool* pool) {
  (void)pool;
  if (left->t == TYPE_INT && is_integer_type(right->t)) {
    return right;
  }
  if (right->t == TYPE_INT && is_integer_type(left->t)) {
    return left;
  }
  if (left->t != right->t) {
    log_err_final("cannot coerce types");
  }
  return left;
}

static void resolve_expr(Expr* expr, MemPool* pool) {
  (void)pool;

  switch (expr->t) {
    case EXPR_INT:
      expr->type = &int_const;
      break;
    case EXPR_VAR:
      expr->type = expr->data.var->inf.type;
      break;
    case EXPR_BINOP:
      resolve_expr(expr->data.binop.left, pool);
      resolve_expr(expr->data.binop.right, pool);
      expr->type = coerce_type(expr->data.binop.left->type,
                               expr->data.binop.right->type, pool);
  }
}

void resolve_types(AST* ast) {
  for (size_t i = 0; i < ast->block.stmts.items; i++) {
    Stmt* temp_stmt = vector_idx(&ast->block.stmts, i);
    switch (temp_stmt->t) {
      case STMT_LET:
        if (temp_stmt->data.let.value) {
          resolve_expr(temp_stmt->data.let.value, &ast->pool);
          if (!temp_stmt->data.let.type) {
            temp_stmt->data.let.type = temp_stmt->data.let.value->type;
            if (temp_stmt->data.let.type->t == TYPE_INT) {
              log_err_final("cannot infer type from integer literals");
            }
          }
        }
        break;
      case STMT_EXPR:
        resolve_expr(temp_stmt->data.expr, &ast->pool);
        break;
    }
  }
}
