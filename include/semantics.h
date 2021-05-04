#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "ast.h"

Type* coerce_type(int op, Type* left, Type* right, MemPool* pool);
void resolve_names(AST* ast);
void resolve_types(AST* ast);
void check_returns(AST* ast);

#endif
