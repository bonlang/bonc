#include "parser.h"

#include <stdlib.h>

#include "ast.h"
#include "lexer.h"

static Scope* cur_scope;

static Expr* make_expr(AST* ast, int t, const uint8_t* start, size_t sz) {
  Expr* ret = mempool_alloc(&ast->pool, sizeof(Expr));
  ret->t = t;
  ret->start = start;
  ret->sz = sz;
  return ret;
}

static Token expect(int t, const char* err_msg) {
  Token ret = lexer_next();
  if (ret.t != t) {
    log_err_final(err_msg);
  }
  return ret;
}

static Expr* parse_expr(AST* ast);

static Expr* parse_primary(AST* ast) {
  Token tok = lexer_next();
  switch (tok.t) {
    case TOK_INT:
      return make_expr(ast, EXPR_INT, tok.start, tok.sz);
    case TOK_SYM:
      return make_expr(ast, EXPR_VAR, tok.start, tok.sz);
    case TOK_LPAREN: {
      Expr* ret = parse_expr(ast);
      expect(TOK_RPAREN, "expected ')'");
      return ret;
    }
    default:
      log_err_final("expected expression");
      return NULL; /* unreachable */
  }
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
    Expr* right = parse_factor(ast);
    Expr* new_ret = make_expr(ast, EXPR_BINOP, ret->start,
                              (right->start - ret->start) + right->sz);
    new_ret->data.binop.left = ret;
    new_ret->data.binop.right = right;
    new_ret->data.binop.op = op;
    ret = new_ret;
  }
  return ret;
}

static inline Expr* parse_expr(AST* ast) { return parse_term(ast); }

static Type U8_const = {.t = TOK_U8};
static Type U16_const = {.t = TOK_U16};
static Type U32_const = {.t = TOK_U32};
static Type U64_const = {.t = TOK_U64};
static Type I8_const = {.t = TOK_I8};
static Type I16_const = {.t = TOK_I16};
static Type I32_const = {.t = TOK_I32};
static Type I64_const = {.t = TOK_I64};
static Type bool_const = {.t = TOK_BOOL};

static void parse_type(AST* ast, Type* type) {
  (void)ast; /* will need this later for allocations */
  Token type_tok = lexer_next();
  switch (type_tok.t) {
    case TOK_U8:
      *type = U8_const;
      break;
    case TOK_U16:
      *type = U16_const;
      break;
    case TOK_U32:
      *type = U32_const;
      break;
    case TOK_U64:
      *type = U64_const;
      break;
    case TOK_I8:
      *type = I8_const;
      break;
    case TOK_I16:
      *type = I16_const;
      break;
    case TOK_I32:
      *type = I32_const;
      break;
    case TOK_I64:
      *type = I64_const;
      break;
    case TOK_BOOL:
      *type = bool_const;
      break;
  }
}
static void parse_let(AST* ast, Stmt* stmt, int mut) {
  lexer_next();
  stmt->t = STMT_LET;
  Token var_name = expect(TOK_SYM, "expected variable name");

  Token middle_tok = lexer_next();
  if (middle_tok.t == TOK_EQ) {
    stmt->data.let.value = parse_expr(ast);
    stmt->data.let.type.t = TYPE_INFER;
    expect(TOK_SEMICOLON, "expected ';'");
  } else if (middle_tok.t == TOK_COLON) {
    parse_type(ast, &stmt->data.let.type);
    Token equal_tok = lexer_next();
    if (equal_tok.t == TOK_EQ) {
      stmt->data.let.value = parse_expr(ast);
      expect(TOK_SEMICOLON, "expected ';'");
    } else if (equal_tok.t == TOK_SEMICOLON) {
      stmt->data.let.value = NULL;
    } else {
      log_err_final("expected '=' or ';'");
    }
  } else {
    log_err_final("expected '=' or ':'");
  }

  stmt->data.let.name = var_name.start;
  stmt->data.let.sz = var_name.sz;
  stmt->data.let.mut = mut;
  VarInfo info = {.mut = mut};
  ScopeEntry* entry =
      scope_insert(&ast->pool, cur_scope, var_name.start, var_name.sz, info);
  if (entry == NULL) {
    log_err_final("redeclaration of variable '%.*s'", (int)var_name.sz,
                  (char*)var_name.start);
  }
}

static void parse_expr_stmt(AST* ast, Stmt* stmt) {
  stmt->t = STMT_EXPR;
  stmt->data.expr = parse_expr(ast);
  expect(TOK_SEMICOLON, "expected ';'");
}

void parse_block(Block* block, AST* ast) {
  vector_init(&block->stmts, sizeof(Stmt), &ast->pool);
  while (1) {
    switch (lexer_peek().t) {
      case TOK_LET:
        parse_let(ast, vector_alloc(&block->stmts, &ast->pool), 0);
        break;
      case TOK_MUT:
        parse_let(ast, vector_alloc(&block->stmts, &ast->pool), 1);
        break;
      /* TODO: Replace this with '}' for proper blocks */
      case TOK_EOF:
        return;
      default:
        parse_expr_stmt(ast, vector_alloc(&block->stmts, &ast->pool));
        break;
    }
  }
}

AST parse_ast() {
  AST ret;

  ast_init(&ret);
  cur_scope = &ret.global;
  parse_block(&ret.block, &ret);
  return ret;
}
