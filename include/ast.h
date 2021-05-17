#ifndef AST_H
#define AST_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "helper.h"
#include "lexer.h"
#include "symtable.h"
#include "type.h"

typedef enum {
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
} BinopKind;

typedef enum {
  EXPR_INT,
  EXPR_VAR,
  EXPR_BINOP,
  EXPR_FUNCALL,
} ExprKind;

typedef struct Expr {
  ExprKind t;
  SourcePosition pos;

  Type *type;
  union {
    struct {
      struct Expr *left;
      struct Expr *right;
      BinopKind op;
    } binop;
    ScopeEntry *var;
    struct {
      uint64_t val;
      IntlitKind type;
    } intlit;
    struct {
      SourcePosition name;
      ScopeEntry *fn;
      Vector args; /* Expr* */
    } funcall;
  } data;

} Expr;

typedef enum {
  STMT_LET,
  STMT_RETURN,
  STMT_EXPR,
} StmtKind;

typedef struct {
  StmtKind t;
  SourcePosition pos;

  union {
    struct {
      SourcePosition name;
      int mut;
      ScopeEntry *var;
      Type *type;
      Expr *value; /* null if variable is not initialized on declaration */
    } let;

    Expr *expr;
    Expr *ret;
  } data;
} Stmt;

typedef struct {
  SourcePosition pos;
  Vector stmts; /* Stmt */
} Block;

typedef struct {
  Type *type;
  SourcePosition name;
  ScopeEntry *entry;
} Param;

typedef struct {
  SourcePosition pos;
  SourcePosition name;
  Block body;
  ScopeEntry *entry;
  Vector params; /* Param */
  Type *ret_type;
  Scope *scope;
} Function;

typedef struct {
  const uint8_t *src_base;
  MemPool pool; /* used to allocate structures that belong to this AST */
  Vector fns;   /* Function */
  Scope *global;
} AST;

void ast_deinit(AST *ast);
void ast_init(AST *ast, const uint8_t *src_base);
void ast_dump(FILE *file, AST *ast);
#endif
