#include "semantics.h"

enum {
  RETURN_RIGHT,
  RETURN_NEVER,
  RETURN_WRONG,
};

static int check_fn(AST* ast, Function* fn) {
  for (size_t i = 0; i < fn->body.stmts.items; i++) {
    Stmt* stmt = vector_idx(&fn->body.stmts, i);
    switch (stmt->t) {
      case STMT_LET:
      case STMT_EXPR:
        break;
      case STMT_RETURN:
        if (stmt->data.ret == NULL) {
          if (fn->ret_type->t == TYPE_VOID) {
            return RETURN_RIGHT;
          } else {
            return RETURN_WRONG;
          }
        }
        return (coerce_type(BINOP_ASSIGN, &fn->ret_type, &stmt->data.ret->type,
                            &ast->pool) != NULL)
                   ? RETURN_RIGHT
                   : RETURN_WRONG;
      default:
        log_internal_err("invalid stmt type %d", stmt->t);
    }
  }
  return (fn->ret_type->t == TYPE_VOID) ? RETURN_RIGHT : RETURN_NEVER;
}

void check_returns(AST* ast) {
  for (size_t i = 0; i < ast->fns.items; i++) {
    Function* fn = vector_idx(&ast->fns, i);
    switch (check_fn(ast, fn)) {
      case RETURN_RIGHT:
        break;
      case RETURN_NEVER:
        log_source_err("non-void function never returns", ast->src_base,
                       fn->pos);
        break;
      case RETURN_WRONG:
        log_source_err("function returns incorrect type", ast->src_base,
                       fn->pos);
        break;
    }
  }
}
