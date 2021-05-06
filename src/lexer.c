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

static inline size_t cur_len() { return lex.end - lex.start; }

static Token make_token(int t) {
  Token ret;
  ret.pos.start = lex.buf + lex.start;
  ret.pos.sz = cur_len();
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

static inline int match(int t, const char* name) {
  if (strlen(name) != cur_len()) {
    return TOK_SYM;
  }
  if (strncmp((char*)name, (char*)lex.buf + lex.start, cur_len()) == 0) {
    return t;
  }
  return TOK_SYM;
}

static int pick_symbol_type() {
  switch (lex.buf[lex.start]) {
    case 't':
      return match(TOK_TRUE, "true");
    case 'r':
      return match(TOK_RETURN, "return");
    case 'f':
      if (cur_len() < 2) {
        return TOK_SYM;
      }
      switch (lex.buf[lex.start + 1]) {
        case 'a':
          return match(TOK_FALSE, "false");
        case 'n':
          return match(TOK_FN, "fn");
      }
      return TOK_SYM;
    case 'b':
      return match(TOK_BOOL, "bool");
    case 'm':
      return match(TOK_MUT, "mut");
    case 'l':
      return match(TOK_LET, "let");
    case 'u':
      if (cur_len() < 2) {
        return TOK_SYM;
      }
      switch (lex.buf[lex.start + 1]) {
        case '8':
          return match(TOK_U8, "u8");
        case '1':
          return match(TOK_U16, "u16");
        case '3':
          return match(TOK_U32, "u32");
        case '6':
          return match(TOK_U64, "u64");
      }
      break;
    case 'i':
      if (cur_len() < 2) {
        return TOK_SYM;
      }
      switch (lex.buf[lex.start + 1]) {
        case '8':
          return match(TOK_I8, "i8");
        case '1':
          return match(TOK_I16, "i16");
        case '3':
          return match(TOK_I32, "i32");
        case '6':
          return match(TOK_I64, "i64");
      }
      break;
  }
  return TOK_SYM;
}

static Token make_symbol() {
  int c;
  while (!is_eof() && (isalpha(c = peek_c()) || isdigit(c) || c == '_')) {
    next_c();
  }
  return make_token(pick_symbol_type());
}

static int next_matches(const char* text) {
  size_t len = strlen(text);
  if (lex.end + len >= lex.sz) {
    return 0;
  }
  if (strncmp(text, (char*)lex.buf + lex.end, len) == 0) {
    lex.end += len;
    return 1;
  }
  return 0;
}

static inline Token make_intlit(int type) {
  Token tok = make_token(TOK_INT);
  tok.intlit_type = type;
  return tok;
}

static Token make_int() {
  while (!is_eof() && isdigit(peek_c())) {
    next_c();
  }
  if (next_matches("i8")) return make_intlit(INTLIT_I8);
  if (next_matches("u8")) return make_intlit(INTLIT_U8);
  if (next_matches("i16")) return make_intlit(INTLIT_I16);
  if (next_matches("u16")) return make_intlit(INTLIT_U16);
  if (next_matches("i32")) return make_intlit(INTLIT_I32);
  if (next_matches("u32")) return make_intlit(INTLIT_U32);
  if (next_matches("i64")) return make_intlit(INTLIT_I64);
  if (next_matches("u64")) return make_intlit(INTLIT_U64);
  return make_intlit(INTLIT_I64_NONE);
}

static Token match_character(int c, int t1, int t2) {
  if (is_eof()) {
    return make_token(t2);
  }

  if (peek_c() == c) {
    next_c();
    return make_token(t1);
  }
  return make_token(t2);
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
      if (is_eof()) {
        return make_token(TOK_EQ);
      }
      if (peek_c() == '=') {
        next_c();
        return make_token(TOK_DEQ);
      }
      return make_token(TOK_EQ);
    case '>':
      return match_character('=', TOK_GREQ, TOK_GR);
    case '<':
      return match_character('=', TOK_LEEQ, TOK_LE);
    case '!':
      return match_character('=', TOK_NOT, TOK_NEQ);
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
    case ':':
      return make_token(TOK_COLON);
    case ',':
      return make_token(TOK_COMMA);
  }
  log_source_err("unexpected char '%c'", lex.buf,
                 make_pos(lex.buf + lex.start, cur_len()), c);
  return make_token(TOK_EOF); /* unreachable */
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
