#include "parser.h"

#include <stdlib.h>

#include "ast.h"
#include "lexer.h"

static Expr* make_expr(AST* ast, int t, const uint8_t* start, size_t sz) {
  Expr* ret = mempool_alloc(&ast->pool, sizeof(Expr));
  ret->t = t;
  ret->start = start;
  ret->sz = sz;
  return ret;
}

static Expr* parse_primary(AST* ast) {
  Token tok = lexer_next();
  if (tok.t != TOK_INT) {
    log_err("expected expression");
  }
  return make_expr(ast, EXPR_INT, tok.start, tok.sz);
}

static int parse_binop() {
  switch (lexer_next().t) {
    case TOK_ADD:
      return BINOP_ADD;
    case TOK_SUB:
      return BINOP_SUB;
    case TOK_MUL:
      return BINOP_MUL;
    case TOK_DIV:
      return BINOP_DIV;
  }
  log_internal_err("impossible binary op token");
  exit(EXIT_FAILURE);
}

static Expr* parse_factor(AST* ast) {
  Expr* ret = parse_primary(ast);
  Token tok;
  while ((tok = lexer_peek()).t == TOK_MUL || tok.t == TOK_DIV) {
    int op = parse_binop();
    Expr* right = parse_primary(ast);
    Expr* new_ret = make_expr(ast, EXPR_BINOP, ret->start,
                              (right->start - ret->start) + right->sz);
    new_ret->data.binop.left = ret;
    new_ret->data.binop.right = right;
    new_ret->data.binop.op = op;
    ret = new_ret;
  }
  return ret;
}

static Expr* parse_term(AST* ast) {
  Expr* ret = parse_factor(ast);
  Token tok;
  while ((tok = lexer_peek()).t == TOK_ADD || tok.t == TOK_SUB) {
    int op = parse_binop();
    Expr* right = parse_term(ast);
    Expr* new_ret = make_expr(ast, EXPR_BINOP, ret->start,
                              (right->start - ret->start) + right->sz);
    new_ret->data.binop.left = ret;
    new_ret->data.binop.right = right;
    new_ret->data.binop.op = op;
    ret = new_ret;
  }
  return ret;
}

static Expr* parse_expr(AST* ast) { return parse_term(ast); }

AST parse_ast() {
  AST ret;

  ast_init(&ret);
  ret.expr = parse_expr(&ret);
  return ret;
}
