#ifndef IR_GEN_H
#define IR_GEN_H

#include "ast.h"
#include "ssa.h"

void translate_function(Function* fn, SSA_BBlock* block, MemPool* pool);

#endif
