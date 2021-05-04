#include "semantics.h"

static void resolve_expr(AST* ast, Scope* scope, Expr* expr) {
  switch (expr->t) {
    case EXPR_BINOP:
      resolve_expr(ast, scope, expr->data.binop.left);
      resolve_expr(ast, scope, expr->data.binop.right);
      break;
    case EXPR_VAR: {
      ScopeEntry* entry = scope_find(scope, expr->start, expr->sz);
      if (entry == NULL) {
        log_source_err("cannot find variable '%.*s'", ast->src_base,
                       expr->start, (int)expr->sz, (char*)expr->start);
      }
      expr->data.var = entry;
    }
  }
}

void resolve_fn(AST* ast, Function* fn) {
  for (size_t i = 0; i < fn->body.stmts.items; i++) {
    Stmt* temp_stmt = vector_idx(&fn->body.stmts, i);
    switch (temp_stmt->t) {
      case STMT_LET:
        if (temp_stmt->data.let.value) {
          resolve_expr(ast, fn->scope, temp_stmt->data.let.value);
        }
        break;
      case STMT_EXPR:
        resolve_expr(ast, fn->scope, temp_stmt->data.expr);
        break;
    }
  }
}
void resolve_names(AST* ast) { resolve_fn(ast, &ast->fn); }
