#include "semantics.h"
#include "error.h"

static void
resolve_expr(AST *ast, Scope *scope, Expr *expr) {
  switch (expr->t) {
    case EXPR_BINOP:
      resolve_expr(ast, scope, expr->data.binop.left);
      resolve_expr(ast, scope, expr->data.binop.right);
      break;
    case EXPR_VAR:
      {
        ScopeEntry *entry = scope_find(scope, expr->pos);
        if (entry == NULL) {
          log_name_not_in_scope(expr->pos, expr->pos);
        }
        expr->data.var = entry;
      }
      break;
    case EXPR_FUNCALL:
      {
        ScopeEntry *entry = scope_find(scope, expr->data.funcall.name);
        if (entry == NULL) {
          log_name_not_in_scope(expr->pos, expr->data.funcall.name);
        }
        for (size_t i = 0; i < expr->data.funcall.args.items; i++) {
          Expr *temp_expr = *((Expr **)vector_idx(&expr->data.funcall.args, i));
          resolve_expr(ast, scope, temp_expr);
        }
        expr->data.funcall.fn = entry;
      }
      break;
    case EXPR_INT:
      break;
    default:
      log_internal_err("invalid expr type %d", expr->t);
  }
}

void
resolve_stmt(AST *ast, Stmt *stmt, Scope *scope) {
  switch (stmt->t) {
    case STMT_LET:
      {
        ScopeEntry *entry = scope_insert(
            &ast->pool, scope, stmt->data.let.name,
            make_var_info(stmt->data.let.mut, stmt->data.let.type));
        if (entry == NULL) {
          log_name_redeclaration(stmt->pos, stmt->data.let.name);
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
    case STMT_RETURN:
      if (stmt->data.ret) {
        resolve_expr(ast, scope, stmt->data.ret);
      }
      break;
    default:
      log_internal_err("invalid stmt type %d", stmt->t);
  }
}
void
resolve_fn(AST *ast, Function *fn) {
  fn->scope = scope_init(&ast->pool, ast->global);
  for (size_t i = 0; i < fn->params.items; i++) {
    Param *param = vector_idx(&fn->params, i);
    param->entry = scope_insert(&ast->pool, fn->scope, param->name,
                                make_var_info(0, param->type));
  }
  for (size_t i = 0; i < fn->body.stmts.items; i++) {
    resolve_stmt(ast, vector_idx(&fn->body.stmts, i), fn->scope);
  }
}

void
resolve_names(AST *ast) {
  ast->global = scope_init(&ast->pool, NULL);
  for (size_t i = 0; i < ast->fns.items; i++) {
    Function *fn = vector_idx(&ast->fns, i);
    fn->entry =
        scope_insert(&ast->pool, ast->global, fn->name, make_var_info(0, NULL));
    if (!fn->entry) {
      log_name_redeclaration(fn->pos, fn->name);
    }
  }
  for (size_t i = 0; i < ast->fns.items; i++) {
    resolve_fn(ast, vector_idx(&ast->fns, i));
  }
}
