#include "semantics.h"

Type* coerce_type(int op, Type** _left, Type** _right, MemPool* pool) {
  Type* left = *_left;
  Type* right = *_right;
  (void)pool;
  switch (op) {
    case BINOP_ADD:
    case BINOP_SUB:
    case BINOP_MUL:
    case BINOP_DIV:
    case BINOP_ASSIGN:
      if (left->t != right->t) {
        return NULL;
      }
      return left;
    case BINOP_NEQ:
    case BINOP_EQ:
    case BINOP_GR:
    case BINOP_GREQ:
    case BINOP_LE:
    case BINOP_LEEQ:
      if (left->t != right->t) {
        return NULL;
      }
      return &bool_const;
    default:
      log_internal_err("invalid binary op %d", op);
      return NULL;
  }
}

static Type* intlit_type_to_type[] = {&I8_const,  &U8_const,  &I16_const,
                                      &U16_const, &I32_const, &U32_const,
                                      &I64_const, &U64_const, &I64_const};

static void resolve_expr(Expr* expr, AST* ast, MemPool* pool) {
  (void)pool;
  switch (expr->t) {
    case EXPR_INT:
      expr->type = intlit_type_to_type[expr->data.intlit.type];
      break;
    case EXPR_VAR:
      expr->type = expr->data.var->inf.type;
      break;
    case EXPR_BINOP:
      resolve_expr(expr->data.binop.left, ast, pool);
      resolve_expr(expr->data.binop.right, ast, pool);
      expr->type =
          coerce_type(expr->data.binop.op, &expr->data.binop.left->type,
                      &expr->data.binop.right->type, pool);
      if (expr->type == NULL) {
        log_source_err("cannot coerce types", ast->src_base, expr->pos);
      }
      break;
    default:
      log_internal_err("invalid expr type %d", expr->t);
  }
}

static void resolve_fn(AST* ast, Function* fn) {
  for (size_t i = 0; i < fn->body.stmts.items; i++) {
    Stmt* temp_stmt = vector_idx(&fn->body.stmts, i);
    switch (temp_stmt->t) {
      case STMT_LET:
        /* is this a composite assignment? */
        if (temp_stmt->data.let.value) {
          resolve_expr(temp_stmt->data.let.value, ast, &ast->pool);
          Type* type = temp_stmt->data.let.value->type;
          /* is this an inferred assignment? */
          if (!temp_stmt->data.let.type) {
            temp_stmt->data.let.type = type;
            temp_stmt->data.let.var->inf.type = type; /* set the symbol table */
          } else if (coerce_type(BINOP_ASSIGN, &temp_stmt->data.let.value->type,
                                 &temp_stmt->data.let.type,
                                 &ast->pool) == NULL) {
            log_source_err("cannot coerce assignment", ast->src_base,
                           temp_stmt->pos);
          }
        }
        break;
      case STMT_EXPR:
        resolve_expr(temp_stmt->data.expr, ast, &ast->pool);
        break;
      case STMT_RETURN:
        if (temp_stmt->data.ret) {
          resolve_expr(temp_stmt->data.ret, ast, &ast->pool);
        }
        break;
      default:
        log_internal_err("invalid stmt type %d", temp_stmt->t);
    }
  }
}

void resolve_types(AST* ast) {
  for (size_t i = 0; i < ast->fns.items; i++) {
    resolve_fn(ast, vector_idx(&ast->fns, i));
  }
}
