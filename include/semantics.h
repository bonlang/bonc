#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "ast.h"

void resolve_names(AST* ast);
void resolve_types(AST* ast);

#endif
