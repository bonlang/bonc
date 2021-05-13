#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdint.h>

#include "helper.h"

enum {
  /* binary ops */
  TOK_ADD,
  TOK_SUB,
  TOK_MUL,
  TOK_DIV,
  TOK_EQ,
  TOK_DEQ, /* == */
  TOK_NEQ,
  TOK_GR,   /* > */
  TOK_LE,   /* < */
  TOK_GREQ, /* >= */
  TOK_LEEQ, /* <= */

  /* unary ops */
  TOK_NOT,

  /* open/closers */
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_LCURLY,
  TOK_RCURLY,
  TOK_LBRACK,
  TOK_RBRACK,

  /* punctuation */
  TOK_COLON,
  TOK_COMMA,

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
  TOK_RETURN,
  TOK_END,

  /* misc. */
  TOK_EOF,
  TOK_NEWLINE,
};

enum {
  INTLIT_I8,
  INTLIT_U8,
  INTLIT_I16,
  INTLIT_U16,
  INTLIT_I32,
  INTLIT_U32,
  INTLIT_I64,
  INTLIT_U64,
  INTLIT_I64_NONE,
};

typedef struct {
  int t;
  int intlit_type;
  SourcePosition pos;
} Token;

void lexer_init(const uint8_t *buf, size_t sz);

Token lexer_next();
Token lexer_peek();

#endif
