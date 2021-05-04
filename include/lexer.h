#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdint.h>

enum {
  /* binary ops */
  TOK_ADD,
  TOK_SUB,
  TOK_MUL,
  TOK_DIV,
  TOK_EQ,

  /* open/closers */
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_LCURLY,
  TOK_RCURLY,
  TOK_LBRACK,
  TOK_RBRACK,

  /* punctuation */
  TOK_SEMICOLON,
  TOK_COLON,

  /* literals/symbols */
  TOK_INT,
  TOK_TRUE,
  TOK_FALSE,
  TOK_SYM,

  /* type names */
  TOK_I8,
  TOK_I16,
  TOK_I32,
  TOK_I64,
  TOK_U8,
  TOK_U16,
  TOK_U32,
  TOK_U64,
  TOK_BOOL,

  /* keywords */
  TOK_LET,
  TOK_MUT,

  /* misc. */
  TOK_EOF,
};

typedef struct {
  int t;
  const uint8_t* start;
  size_t sz;
} Token;

void lexer_init(const uint8_t* buf, size_t sz);

Token lexer_next();
Token lexer_peek();

#endif
