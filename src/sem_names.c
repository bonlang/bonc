#include "semantics.h"

static void resolve_expr(AST* ast, Scope* scope, Expr* expr) {
  switch (expr->t) {
    case EXPR_BINOP:
      resolve_expr(ast, scope, expr->data.binop.left);
      resolve_expr(ast, scope, expr->data.binop.right);
      break;
    case EXPR_VAR: {
      ScopeEntry* entry = scope_find(scope, expr->pos);
      if (entry == NULL) {
        log_source_err("cannot find variable '%.*s'", ast->src_base, expr->pos);
      }
      expr->data.var = entry;
    }
  }
}

void resolve_stmt(AST* ast, Stmt* stmt, Scope* scope) {
  switch (stmt->t) {
    case STMT_LET: {
      VarInfo info = {.mut = stmt->data.let.mut, .type = stmt->data.let.type};
      ScopeEntry* entry =
          scope_insert(&ast->pool, scope, stmt->data.let.name, info);
      if (entry == NULL) {
        log_source_err("cannot redeclare variable '%.*s'", ast->src_base,
                       stmt->pos, (int)stmt->data.let.name.sz,
                       (char*)stmt->data.let.name.start);
      }
      stmt->data.let.var = entry;
      if (stmt->data.let.value) {
        resolve_expr(ast, scope, stmt->data.let.value);
      }
      break;
    }
    case STMT_EXPR:
      resolve_expr(ast, scope, stmt->data.expr);
      break;
  }
}
void resolve_fn(AST* ast, Function* fn) {
  fn->scope = scope_init(&ast->pool, ast->global);
  for (size_t i = 0; i < fn->body.stmts.items; i++) {
    resolve_stmt(ast, vector_idx(&fn->body.stmts, i), fn->scope);
  }
}
void resolve_names(AST* ast) {
  ast->global = scope_init(&ast->pool, NULL);
  resolve_fn(ast, &ast->fn);
}
