#include "semantics.h"

static int check_fn(AST* ast, Function* fn) {
  for (size_t i = 0; i < fn->body.stmts.items; i++) {
    Stmt* stmt = vector_idx(&fn->body.stmts, i);
    switch (stmt->t) {
      case STMT_LET:
      case STMT_EXPR:
        break;
      case STMT_RETURN:
        if (fn->ret_type->t == TYPE_VOID && stmt->data.ret == NULL) {
          return 1;
        }
        return coerce_type(BINOP_ASSIGN, &fn->ret_type, &stmt->data.ret->type,
                           &ast->pool) != NULL;
      default:
        log_internal_err("invalid stmt type %d", stmt->t);
    }
  }
  return fn->ret_type->t == TYPE_VOID;
}

void check_returns(AST* ast) {
  for (size_t i = 0; i < ast->fns.items; i++) {
    Function* fn = vector_idx(&ast->fns, i);
    if (!check_fn(ast, fn)) {
      log_source_err("non-void function never returns", ast->src_base, fn->pos);
    }
  }
}
