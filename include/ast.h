#ifndef AST_H
#define AST_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "helper.h"
#include "symtable.h"
#include "type.h"

enum {
  BINOP_ADD,
  BINOP_SUB,
  BINOP_MUL,
  BINOP_DIV,
  BINOP_EQ,
  BINOP_NEQ,
  BINOP_GR,
  BINOP_LE,
  BINOP_GREQ,
  BINOP_LEEQ,

  /* only used by coerce_type */
  BINOP_ASSIGN,
};

enum {
  EXPR_INT,
  EXPR_VAR,
  EXPR_BINOP,
};

typedef struct Expr {
  int t;
  SourcePosition pos;

  Type* type;
  union {
    struct {
      struct Expr* left;
      struct Expr* right;
      int op;
    } binop;
    ScopeEntry* var;
    struct {
      SourcePosition literal;
      int type;
    } intlit;
  } data;

} Expr;

enum {
  STMT_LET,
  STMT_RETURN,
  STMT_EXPR,
};

typedef struct {
  int t;
  SourcePosition pos;

  union {
    struct {
      SourcePosition name;
      int mut;
      ScopeEntry* var;
      Type* type;
      Expr* value; /* null if variable is not initialized on declaration */
    } let;

    Expr* expr;
    Expr* ret;
  } data;
} Stmt;

typedef struct {
  SourcePosition pos;
  Vector stmts; /* Stmt */
} Block;

typedef struct {
  Type* type;
  SourcePosition name;
} Param;

typedef struct {
  SourcePosition pos;
  Block body;
  Vector params; /* Param */
  Type* ret_type;
  Scope* scope;
} Function;

typedef struct {
  const uint8_t* src_base;
  MemPool pool; /* used to allocate structures that belong to this AST */
  Vector fns;   /* Function */
  Scope* global;
} AST;

void ast_deinit(AST* ast);
void ast_init(AST* ast, const uint8_t* src_base);
void ast_dump(FILE* file, AST* ast);
#endif
