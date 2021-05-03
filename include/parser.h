#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

/* lexer must be initialized previous to calling this function */
AST parse_ast();

#endif
