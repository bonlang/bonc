#include "lexer.h"

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
  return ret;
}

static Token lexer_fetch() { return make_token(TOK_EOF); }

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
