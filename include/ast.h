#ifndef AST_H
#define AST_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "helper.h"
#include "symtable.h"

enum {
  BINOP_ADD,
  BINOP_SUB,
  BINOP_MUL,
  BINOP_DIV,
};

enum {
  EXPR_INT,
  EXPR_VAR,
  EXPR_BINOP,
  EXPR_DEC,
};

typedef struct Expr {
  int t;
  const uint8_t* start;
  size_t sz;

  union {
    struct {
      struct Expr* left;
      struct Expr* right;
      int op;
    } binop;
    struct {
      const char* name;
      size_t sz;
      int mut;
      struct Expr* expr;
    } dec;
  } data;

} Expr;

typedef struct {
  MemPool pool; /* used to allocate structures that belong to this AST */
  Expr* expr;
  Scope global;
} AST;

void ast_deinit(AST* ast);
void ast_init(AST* ast);
void ast_dump(FILE* file, AST* ast);
#endif
