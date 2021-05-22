#include "semantics.h"
#include "error.h"

enum {
  RETURN_RIGHT,
  RETURN_NEVER,
  RETURN_WRONG,
};

/* first_wrong_stmt, and first_wrong_type are set if the function returns
 * incorrectly */
/* this method will make it easier to identify incorrect returns and properly
 * fill in error info on the AST later */
static int
check_fn(AST *ast, Function *fn, Stmt **first_wrong_stmt,
         Type **first_wrong_type) {
  for (size_t i = 0; i < fn->body.stmts.items; i++) {
    Stmt *stmt = vector_idx(&fn->body.stmts, i);
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
        if (coerce_type(BINOP_ASSIGN, &fn->ret_type, &stmt->data.ret->type,
                        &ast->pool) != NULL) {
          return RETURN_RIGHT;
        } else {
          *first_wrong_stmt = stmt;
          *first_wrong_type = stmt->data.ret->type;
          return RETURN_WRONG;
        }

      default:
        log_internal_err("invalid stmt type %d", stmt->t);
    }
  }
  return (fn->ret_type->t == TYPE_VOID) ? RETURN_RIGHT : RETURN_NEVER;
}

void
check_returns(AST *ast) {
  Stmt *wrong_stmt;
  Type *wrong_type;
  for (size_t i = 0; i < ast->fns.items; i++) {
    Function *fn = vector_idx(&ast->fns, i);
    switch (check_fn(ast, fn, &wrong_stmt, &wrong_type)) {
      case RETURN_RIGHT:
        break;
      case RETURN_NEVER:
        log_never_returns(fn->pos);
        break;
      case RETURN_WRONG:
        log_incorrect_return(wrong_stmt->pos, fn->ret_type, wrong_type);
        break;
    }
  }
}
