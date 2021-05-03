#include "parser.h"

#include "ast.h"
#include "lexer.h"

Expr* make_expr(AST* ast, int t, const uint8_t* start, size_t sz) {
  Expr* ret = mempool_alloc(&ast->pool, sizeof(Expr));
  ret->t = t;
  ret->start = start;
  ret->sz = sz;
  return ret;
}

Expr* parse_primary(AST* ast) {
  Token tok = lexer_next();
  if (tok.t != TOK_INT) {
    log_err("expected expression");
  }
  return make_expr(ast, EXPR_INT, tok.start, tok.sz);
}

Expr* parse_expr(AST* ast) { return parse_primary(ast); }

AST parse_ast() {
  AST ret;

  ast_init(&ret);
  ret.expr = parse_expr(&ret);
  return ret;
}
