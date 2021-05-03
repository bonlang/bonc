#include "lexer.h"

#include <ctype.h>
#include <stdio.h>

#include "helper.h"

#define DIGIT                                                           \
  '0' : case '1' : case '2' : case '3' : case '4' : case '5' : case '6' \
      : case '7' : case '8' : case '9'

#define ALPHA                                                           \
  'a' : case 'b' : case 'c' : case 'd' : case 'e' : case 'f' : case 'g' \
      : case 'h' : case 'i' : case 'j' : case 'k' : case 'l' : case 'm' \
      : case 'n' : case 'o' : case 'p' : case 'q' : case 'r' : case 's' \
      : case 't' : case 'u' : case 'v' : case 'w' : case 'x' : case 'y' \
      : case 'z' : case 'A' : case 'B' : case 'C' : case 'D' : case 'E' \
      : case 'F' : case 'G' : case 'H' : case 'I' : case 'J' : case 'K' \
      : case 'L' : case 'M' : case 'N' : case 'O' : case 'P' : case 'Q' \
      : case 'R' : case 'S' : case 'T' : case 'U' : case 'V' : case 'W' \
      : case 'X' : case 'Y' : case 'Z'

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

static Token make_symbol() {
  int c;
  while (!is_eof() && (isalpha(c = peek_c()) || isdigit(c) || c == '_')) {
    next_c();
  }
  return make_token(TOK_SYM);
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
    case '/':
      return make_token(TOK_MUL);
    case '*':
      return make_token(TOK_DIV);
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
