#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "helper.h"

struct {
  const uint8_t* buf;
  size_t sz;

  size_t end;
  size_t start;

  Token peek;
  int peekf; /* 0 if no token available to peek */
} lex;

void lexer_init(const uint8_t* buf, size_t sz) {
  lex.buf = buf;
  lex.sz = sz;

  lex.end = lex.start = 0;

  lex.peekf = 0;
}

static Token make_token(int t) {
  Token ret;
  ret.start = lex.buf + lex.start;
  ret.sz = lex.end - lex.start;
  ret.t = t;
  lex.start = lex.end; /* reset lexer */
  return ret;
}

static int is_eof() { return lex.end >= lex.sz; }

static inline int is_whitespace(int32_t c) {
  return c == ' ' || c == '\n' || c == '\t';
}

static inline uint8_t next_c() { return lex.buf[lex.end++]; }

static inline uint8_t peek_c() { return lex.buf[lex.end]; }

static void skip_whitespace() {
  while (!is_eof() && is_whitespace(peek_c())) {
    next_c();
  }
  lex.start = lex.end;
}

static inline size_t cur_len() { return lex.end - lex.start; }

static inline int match(int t, const char* name) {
  if (strncmp((char*)name, (char*)lex.buf + lex.start, cur_len()) == 0) {
    return t;
  }
  return TOK_SYM;
}

static int pick_symbol_type() {
  switch (lex.buf[lex.start]) {
    case 'm':
      return match(TOK_MUT, "mut");
    case 'l':
      return match(TOK_LET, "let");
    default:
      return TOK_SYM;
  }
}

static Token make_symbol() {
  int c;
  while (!is_eof() && (isalpha(c = peek_c()) || isdigit(c) || c == '_')) {
    next_c();
  }
  return make_token(pick_symbol_type());
}

static Token make_int() {
  while (!is_eof() && isdigit(peek_c())) {
    next_c();
  }
  return make_token(TOK_INT);
}

static Token lexer_fetch() {
  skip_whitespace();

  if (is_eof()) return make_token(TOK_EOF);

  int c = next_c();
  if (isdigit(c)) {
    return make_int();
  }
  if (isalpha(c) || c == '_') {
    return make_symbol();
  }
  switch (c) {
    case '+':
      return make_token(TOK_ADD);
    case '-':
      return make_token(TOK_SUB);
    case '*':
      return make_token(TOK_MUL);
    case '/':
      return make_token(TOK_DIV);
    case '=':
      return make_token(TOK_EQ);
    case '(':
      return make_token(TOK_LPAREN);
    case ')':
      return make_token(TOK_RPAREN);
    case '{':
      return make_token(TOK_LCURLY);
    case '}':
      return make_token(TOK_RCURLY);
    case '[':
      return make_token(TOK_LBRACK);
    case ']':
      return make_token(TOK_RBRACK);
    case ';':
      return make_token(TOK_SEMICOLON);
  }
  log_err_final("unexpected char '%c'", c);
  return make_symbol(TOK_EOF); /* unreachable */
}

Token lexer_next() {
  if (lex.peekf) {
    lex.peekf = 0;
    return lex.peek;
  }
  return lexer_fetch();
}

Token lexer_peek() {
  if (lex.peekf) {
    return lex.peek;
  }
  Token ret = lexer_fetch();
  lex.peekf = 1;
  lex.peek = ret;
  return ret;
}
