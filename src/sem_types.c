#include "semantics.h"

static inline int is_integer_type(int t) {
  return t == TYPE_U8 || t == TYPE_U16 || t == TYPE_U32 || t == TYPE_U64 ||
         t == TYPE_I8 || t == TYPE_I16 || t == TYPE_I32 || t == TYPE_I64 ||
         t == TYPE_INT;
}

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
      if (left->t == TYPE_INT && is_integer_type(right->t)) {
        *_left = right;
        return right;
      }
      if (right->t == TYPE_INT && is_integer_type(left->t)) {
        *_right = left;
        return left;
      }
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
      if (left->t == TYPE_INT && is_integer_type(right->t)) {
        *_left = right;
        return &bool_const;
      }
      if (right->t == TYPE_INT && is_integer_type(left->t)) {
        *_right = left;
        return &bool_const;
      }
      if (left->t != right->t) {
        return NULL;
      }
      return &bool_const;
    default:
      log_internal_err("invalid binary op");
      return NULL;
  }
}

static void resolve_expr(Expr* expr, AST* ast, MemPool* pool) {
  (void)pool;

  switch (expr->t) {
    case EXPR_INT:
      expr->type = &int_const;
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
  }
}

void resolve_types(AST* ast) {
  for (size_t i = 0; i < ast->fn.body.stmts.items; i++) {
    Stmt* temp_stmt = vector_idx(&ast->fn.body.stmts, i);
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
            if (temp_stmt->data.let.type->t == TYPE_INT) {
              log_err_final("cannot infer type from integer literals");
            }
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
    }
  }
}
