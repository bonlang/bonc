#include "parser.h"

#include <stdlib.h>

#include "ast.h"
#include "lexer.h"

const uint8_t *src_base;

static Expr *
make_expr(AST *ast, int t, SourcePosition pos) {
  Expr *ret = mempool_alloc(&ast->pool, sizeof(Expr));
  ret->t = t;
  ret->pos = pos;
  ret->type = NULL;
  return ret;
}

static Token
expect(int t, const char *err_msg) {
  Token ret = lexer_next();
  if (ret.t != t) {
    log_source_err(err_msg, src_base, ret.pos);
  }
  return ret;
}

static Expr *parse_expr(AST *ast);

static const size_t intlit_pos_sz[] = {2, 2, 3, 3, 3, 3, 3, 3, 0};

static inline Expr *
make_intlit_expr(AST *ast, SourcePosition whole_pos, int t) {
  Expr *ret = make_expr(ast, EXPR_INT, whole_pos);
  ret->data.intlit.literal = whole_pos;
  ret->data.intlit.literal.sz -= intlit_pos_sz[t];
  ret->data.intlit.type = t;
  return ret;
}

static Expr *
parse_funcall(AST *ast, Token name_tok) {
  lexer_next();
  Vector args;
  vector_init(&args, sizeof(Expr *), &ast->pool);
  while (lexer_peek().t != TOK_RPAREN) {
    Expr *temp = parse_expr(ast);
    vector_push(&args, &temp, &ast->pool);
    if (lexer_peek().t != TOK_COMMA) {
      break;
    }
    lexer_next();
  }
  Token last_paren = expect(TOK_RPAREN, "expected ')'");
  Expr *ret =
      make_expr(ast, EXPR_FUNCALL, combine_pos(name_tok.pos, last_paren.pos));
  ret->data.funcall.args = args;
  ret->data.funcall.name = name_tok.pos;
  return ret;
}

static Expr *
parse_primary(AST *ast) {
  Token tok = lexer_next();
  switch (tok.t) {
    case TOK_INT:
      return make_intlit_expr(ast, tok.pos, tok.intlit_type);
    case TOK_SYM:
      if (lexer_peek().t == TOK_LPAREN) {
        return parse_funcall(ast, tok);
      }
      return make_expr(ast, EXPR_VAR, tok.pos);
    case TOK_LPAREN:
      {
        Expr *ret = parse_expr(ast);
        expect(TOK_RPAREN, "expected ')'");
        return ret;
      }
    default:
      log_source_err("expected expression", src_base, tok.pos);
      return NULL; /* unreachable */
  }
}

static int
parse_binop() {
  Token tok = lexer_next();
  switch (tok.t) {
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
  log_internal_err("impossible binary op token %d", tok.t);
  exit(EXIT_FAILURE);
}

static Expr *
parse_factor(AST *ast) {
  Expr *ret = parse_primary(ast);
  Token tok;
  while ((tok = lexer_peek()).t == TOK_MUL || tok.t == TOK_DIV) {
    int op = parse_binop();
    Expr *right = parse_primary(ast);
    Expr *new_ret =
        make_expr(ast, EXPR_BINOP, combine_pos(ret->pos, right->pos));
    new_ret->data.binop.left = ret;
    new_ret->data.binop.right = right;
    new_ret->data.binop.op = op;
    ret = new_ret;
  }
  return ret;
}

static Expr *
parse_term(AST *ast) {
  Expr *ret = parse_factor(ast);
  Token tok;
  while ((tok = lexer_peek()).t == TOK_ADD || tok.t == TOK_SUB) {
    int op = parse_binop();
    Expr *right = parse_factor(ast);
    Expr *new_ret =
        make_expr(ast, EXPR_BINOP, combine_pos(ret->pos, right->pos));
    new_ret->data.binop.left = ret;
    new_ret->data.binop.right = right;
    new_ret->data.binop.op = op;
    ret = new_ret;
  }
  return ret;
}

static Expr *
parse_comp(AST *ast) {
  Expr *ret = parse_term(ast);
  Token tok;
  while ((tok = lexer_peek()).t == TOK_DEQ || tok.t == TOK_NEQ ||
         tok.t == TOK_GR || tok.t == TOK_LE || tok.t == TOK_GREQ ||
         tok.t == TOK_LEEQ) {
    int op = parse_binop();
    Expr *right = parse_term(ast);
    Expr *new_ret =
        make_expr(ast, EXPR_BINOP, combine_pos(ret->pos, right->pos));
    new_ret->data.binop.left = ret;
    new_ret->data.binop.right = right;
    new_ret->data.binop.op = op;
    ret = new_ret;
  }
  return ret;
}

static inline Expr *
parse_expr(AST *ast) {
  return parse_comp(ast);
}

static Type *
parse_type(AST *ast) {
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
  log_source_err("expected type name", src_base, type_tok.pos);
  return NULL;
}

static void
parse_let(AST *ast, Stmt *stmt, int mut) {
  Token first_tok = lexer_next();
  stmt->t = STMT_LET;
  Token var_name = expect(TOK_SYM, "expected variable name");

  Token last_tok;
  Token middle_tok = lexer_next();
  if (middle_tok.t == TOK_EQ) {
    stmt->data.let.value = parse_expr(ast);
    stmt->data.let.type = NULL;
    last_tok = expect(TOK_SEMICOLON, "expected ';'");
  } else if (middle_tok.t == TOK_COLON) {
    stmt->data.let.type = parse_type(ast);
    Token equal_tok = lexer_next();
    if (equal_tok.t == TOK_EQ) {
      stmt->data.let.value = parse_expr(ast);
      last_tok = expect(TOK_SEMICOLON, "expected ';'");
    } else if (equal_tok.t == TOK_SEMICOLON) {
      last_tok = equal_tok;
      stmt->data.let.value = NULL;
    } else {
      log_source_err("expected '=' or ';'", src_base, equal_tok.pos);
    }
  } else {
    log_source_err("expected '=' or ':'", src_base, middle_tok.pos);
  }

  stmt->pos = combine_pos(first_tok.pos, last_tok.pos);
  stmt->data.let.name = var_name.pos;
  stmt->data.let.mut = mut;
}

static void
parse_expr_stmt(AST *ast, Stmt *stmt) {
  stmt->t = STMT_EXPR;
  stmt->data.expr = parse_expr(ast);
  expect(TOK_SEMICOLON, "expected ';'");
}

static void
parse_return(AST *ast, Stmt *stmt) {
  lexer_next(); /* skip 'return' */
  stmt->t = STMT_RETURN;
  if (lexer_peek().t == TOK_SEMICOLON) {
    lexer_next();
    stmt->data.ret = NULL;
  } else {
    stmt->data.ret = parse_expr(ast);
    expect(TOK_SEMICOLON, "expected ';'");
  }
}

void
parse_block(Block *block, AST *ast) {
  lexer_next(); /* skip '{' */
  vector_init(&block->stmts, sizeof(Stmt), &ast->pool);
  while (1) {
    switch (lexer_peek().t) {
      case TOK_LET:
        parse_let(ast, vector_alloc(&block->stmts, &ast->pool), 0);
        break;
      case TOK_RETURN:
        parse_return(ast, vector_alloc(&block->stmts, &ast->pool));
        break;
      case TOK_MUT:
        parse_let(ast, vector_alloc(&block->stmts, &ast->pool), 1);
        break;
      /* TODO: Replace this with '}' for proper blocks */
      case TOK_RCURLY:
        lexer_next();
        return;
      default:
        parse_expr_stmt(ast, vector_alloc(&block->stmts, &ast->pool));
        break;
    }
  }
}

void
parse_fn(AST *ast, Function *function) {
  expect(TOK_FN, "expected 'fn'");

  Token name_tok = expect(TOK_SYM, "expected function name");
  function->name = name_tok.pos;
  function->pos = name_tok.pos;

  expect(TOK_LPAREN, "expected '('");
  vector_init(&function->params, sizeof(Param), &ast->pool);

  while (lexer_peek().t != TOK_RPAREN) {
    Param *param = vector_alloc(&function->params, &ast->pool);
    Token name_tok = expect(TOK_SYM, "expected param name");
    param->name = name_tok.pos;

    expect(TOK_COLON, "expected ':'");
    param->type = parse_type(ast);
    if (lexer_peek().t == TOK_COMMA) {
      lexer_next();
    }
  }
  lexer_next(); /* skip ')' */

  function->ret_type = &void_const;
  if (lexer_peek().t != TOK_LCURLY) {
    function->ret_type = parse_type(ast);
  }

  parse_block(&function->body, ast);
}

AST
parse_ast(const uint8_t *src) {
  AST ret;

  ast_init(&ret, src);
  ast_init(&ret, src);

  while (lexer_peek().t != TOK_EOF) {
    parse_fn(&ret, vector_alloc(&ret.fns, &ret.pool));
  }

  src_base = src;
  return ret;
}
