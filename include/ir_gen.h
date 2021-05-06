#ifndef IR_GEN_H
#define IR_GEN_H

#include "ast.h"
#include "ssa.h"

void translate_ast(AST* ast, SSA_Prog* prog, MemPool* pool);

#endif
