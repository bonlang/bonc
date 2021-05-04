#ifndef AST_H
#define AST_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "helper.h"
#include "symtable.h"

enum {
  TYPE_INFER, /* used in let stmt to say the type is inferred */
  TYPE_U8,
  TYPE_U16,
  TYPE_U32,
  TYPE_U64,
  TYPE_I8,
  TYPE_I16,
  TYPE_I32,
  TYPE_I64,
  TYPE_BOOL,
};

typedef struct Type {
  int t;
} Type;

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
    ScopeEntry* var;
  } data;

} Expr;

enum {
  STMT_LET,
  STMT_EXPR,
};

typedef struct {
  int t;

  union {
    struct {
      const uint8_t* name;
      size_t sz;
      int mut;
      Type type;
      Expr* value; /* null if variable is not initialized on declaration */
    } let;

    Expr* expr;
  } data;
} Stmt;

typedef struct {
  Vector stmts; /* Stmt */
} Block;

typedef struct {
  MemPool pool; /* used to allocate structures that belong to this AST */
  Block block;
  Scope global;
} AST;

void ast_deinit(AST* ast);
void ast_init(AST* ast);
void ast_dump(FILE* file, AST* ast);
#endif
