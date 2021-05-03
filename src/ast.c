#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

void ast_deinit(AST* ast) { mempool_deinit(&ast->pool); }

void ast_init(AST* ast) {
  mempool_init(&ast->pool);
  scope_init(&ast->global, &ast->pool, NULL);
}

static void print_indent(FILE* file, int indent) {
  for (; indent > 0; indent--) {
    fprintf(file, "    ");
  }
}

static const char* str_of_binop(int op) {
  switch (op) {
    case BINOP_ADD:
      return "+";
    case BINOP_SUB:
      return "-";
    case BINOP_MUL:
      return "*";
    case BINOP_DIV:
      return "/";
  }
  log_internal_err("invalid binary op: %d", op);
  exit(EXIT_FAILURE);
}

static void expr_dump(FILE* file, Expr* expr, int indent) {
  print_indent(file, indent);
  switch (expr->t) {
    case EXPR_INT:
      fprintf(file, "Expr_Int: %.*s\n", (int)expr->sz, (char*)expr->start);
      break;
    case EXPR_VAR:
      fprintf(file, "Expr_Var: %.*s\n", (int)expr->sz, (char*)expr->start);
      break;
    case EXPR_BINOP:
      fprintf(file, "Expr_Binop: %s\n", str_of_binop(expr->data.binop.op));
      expr_dump(file, expr->data.binop.left, indent + 1);
      expr_dump(file, expr->data.binop.right, indent + 1);
      break;
  }
}

static void stmt_dump(FILE* file, Stmt* stmt) {
  switch (stmt->t) {
    case STMT_LET:
      fprintf(file, "Stmt_Let: %.*s\n", (int)stmt->data.let.sz,
              (char*)stmt->data.let.name);
      expr_dump(file, stmt->data.let.value, 1);
      break;
    case STMT_EXPR:
      fprintf(file, "Stmt_Expr:\n");
      expr_dump(file, stmt->data.expr, 1);
      break;
  }
}

void ast_dump(FILE* file, AST* ast) {
  for (size_t i = 0; i < ast->block.stmts.items; i++) {
    stmt_dump(file, vector_idx(&ast->block.stmts, i));
  }
}

