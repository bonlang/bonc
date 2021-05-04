#include "semantics.h"

void resolve_name(Expr* expr, MemPool* pool) {
  (void)pool;

  switch (expr->t) {
    case EXPR_INT:
      expr->type = &I64_const;
      break;
    case EXPR_VAR:
      expr->type = expr->data.var->inf.type;
      break;
  }
}

void resolve_types(AST* ast) {
  for (size_t i = 0; i < ast->block.stmts.items; i++) {
    Stmt* temp_stmt = vector_idx(&ast->block.stmts, i);
    switch (temp_stmt->t) {
      case STMT_LET:
        if (temp_stmt->data.let.value) {
          resolve_name(temp_stmt->data.let.value, &ast->pool);
          if (!temp_stmt->data.let.type) {
            temp_stmt->data.let.type = temp_stmt->data.let.value->type;
          }
        }
        break;
      case STMT_EXPR:
        resolve_name(temp_stmt->data.expr, &ast->pool);
        break;
    }
  }
}
