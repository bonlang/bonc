#include "semantics.h"

Type *
coerce_type(int op, Type **_left, Type **_right, MemPool *pool) {
  Type *left = *_left;
  Type *right = *_right;
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

static Type *intlit_type_to_type[] = {&I8_const,  &U8_const,  &I16_const,
                                      &U16_const, &I32_const, &U32_const,
                                      &I64_const, &U64_const, &I64_const};

static void
resolve_expr(Expr *expr, AST *ast, MemPool *pool) {
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
    case EXPR_FUNCALL:
      {
        Type *fn_type = expr->data.funcall.fn->inf.type;
        if (fn_type->t != TYPE_FN) {
          log_source_err("cannot call a non-function value", ast->src_base,
                         expr->pos);
        }
        if (fn_type->data.fn.args.items != expr->data.funcall.args.items) {
          log_source_err("too %s parameters given in function call",
                         ast->src_base, expr->pos,
                         fn_type->data.fn.args.items >
                                 expr->data.funcall.args.items
                             ? "few"
                             : "many");
        }
        for (size_t i = 0; i < fn_type->data.fn.args.items; i++) {
          Type **expected = vector_idx(&fn_type->data.fn.args, i);
          Expr *temp_expr = *((Expr **)vector_idx(&expr->data.funcall.args, i));
          resolve_expr(temp_expr, ast, pool);
          Type **given = &temp_expr->type;
          if (coerce_type(BINOP_ASSIGN, expected, given, pool) == NULL) {
            log_source_err("cannot coerce parameter", ast->src_base,
                           temp_expr->pos);
          }
        }
        expr->type = fn_type->data.fn.ret;
        break;
      }
    default:
      log_internal_err("invalid expr type %d", expr->t);
  }
}

static void
resolve_fn(AST *ast, Function *fn) {
  for (size_t i = 0; i < fn->body.stmts.items; i++) {
    Stmt *temp_stmt = vector_idx(&fn->body.stmts, i);
    switch (temp_stmt->t) {
      case STMT_LET:
        /* is this a composite assignment? */
        if (temp_stmt->data.let.value) {
          resolve_expr(temp_stmt->data.let.value, ast, &ast->pool);
          Type *type = temp_stmt->data.let.value->type;
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

static Type *
build_fn_type(AST *ast, Function *fn) {
  Type *fn_type = mempool_alloc(&ast->pool, sizeof(Type));
  fn_type->t = TYPE_FN;
  fn_type->data.fn.ret = fn->ret_type;

  vector_init(&fn_type->data.fn.args, sizeof(Type *), &ast->pool);
  for (size_t i = 0; i < fn->params.items; i++) {
    Param *param = vector_idx(&fn->params, i);
    vector_push(&fn_type->data.fn.args, &param->type);
  }

  return fn_type;
}

void
resolve_types(AST *ast) {
  for (size_t i = 0; i < ast->fns.items; i++) {
    Function *fn = vector_idx(&ast->fns, i);
    Type *fn_type = build_fn_type(ast, fn);
    fn->entry->inf.type = fn_type;
  }

  for (size_t i = 0; i < ast->fns.items; i++) {
    resolve_fn(ast, vector_idx(&ast->fns, i));
  }
}
