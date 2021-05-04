#include "parser.h"

#include <stdlib.h>

#include "ast.h"
#include "lexer.h"

static Scope* cur_scope;
const uint8_t* src_base;

static Expr* make_expr(AST* ast, int t, const uint8_t* start, size_t sz) {
  Expr* ret = mempool_alloc(&ast->pool, sizeof(Expr));
  ret->t = t;
  ret->start = start;
  ret->sz = sz;
  ret->type = NULL;
  return ret;
}

static Token expect(int t, const char* err_msg) {
  Token ret = lexer_next();
  if (ret.t != t) {
    log_source_err(err_msg, src_base, ret.start);
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
      log_source_err("expected expression", src_base, tok.start);
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
    case TOK_DEQ:
      return BINOP_EQ;
    case TOK_NEQ:
      return BINOP_NEQ;
    case TOK_GR:
      return BINOP_GR;
    case TOK_LE:
      return BINOP_LE;
    case TOK_GREQ:
      return BINOP_GREQ;
    case TOK_LEEQ:
      return BINOP_LEEQ;
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

static Expr* parse_comp(AST* ast) {
  Expr* ret = parse_term(ast);
  Token tok;
  while ((tok = lexer_peek()).t == TOK_DEQ || tok.t == TOK_NEQ ||
         tok.t == TOK_GR || tok.t == TOK_LE || tok.t == TOK_GREQ ||
         tok.t == TOK_LEEQ) {
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

static inline Expr* parse_expr(AST* ast) { return parse_comp(ast); }

static Type* parse_type(AST* ast) {
  (void)ast; /* will need this later for allocations */
  Token type_tok = lexer_next();
  switch (type_tok.t) {
    case TOK_U8:
      return &U8_const;
    case TOK_U16:
      return &U16_const;
    case TOK_U32:
      return &U32_const;
    case TOK_U64:
      return &U64_const;
    case TOK_I8:
      return &I8_const;
    case TOK_I16:
      return &I16_const;
    case TOK_I32:
      return &I32_const;
    case TOK_I64:
      return &I64_const;
    case TOK_BOOL:
      return &bool_const;
  }
  log_source_err("expected type name", src_base, type_tok.start);
  return NULL;
}

static void parse_let(AST* ast, Stmt* stmt, int mut) {
  lexer_next();
  stmt->t = STMT_LET;
  Token var_name = expect(TOK_SYM, "expected variable name");

  Token middle_tok = lexer_next();
  if (middle_tok.t == TOK_EQ) {
    stmt->data.let.value = parse_expr(ast);
    stmt->data.let.type = NULL;
    expect(TOK_SEMICOLON, "expected ';'");
  } else if (middle_tok.t == TOK_COLON) {
    stmt->data.let.type = parse_type(ast);
    Token equal_tok = lexer_next();
    if (equal_tok.t == TOK_EQ) {
      stmt->data.let.value = parse_expr(ast);
      expect(TOK_SEMICOLON, "expected ';'");
    } else if (equal_tok.t == TOK_SEMICOLON) {
      stmt->data.let.value = NULL;
    } else {
      log_source_err("expected '=' or ';'", src_base, equal_tok.start);
    }
  } else {
    log_source_err("expected '=' or ':'", src_base, middle_tok.start);
  }

  stmt->data.let.name = var_name.start;
  stmt->data.let.sz = var_name.sz;
  stmt->data.let.mut = mut;
  VarInfo info = {.mut = mut, .type = stmt->data.let.type};
  ScopeEntry* entry =
      scope_insert(&ast->pool, cur_scope, var_name.start, var_name.sz, info);
  if (entry == NULL) {
    log_source_err("redeclaration of variable '%.*s'", src_base, var_name.start,
                   (int)var_name.sz, (char*)var_name.start);
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

AST parse_ast(const uint8_t* src) {
  AST ret;

  ast_init(&ret, src);
  src_base = src;
  cur_scope = &ret.global;
  parse_block(&ret.block, &ret);
  return ret;
}
