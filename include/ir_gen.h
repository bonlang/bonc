#ifndef IR_GEN_H
#define IR_GEN_H

#include "ast.h"
#include "ssa.h"

SSA_BBlock* translate_function(Function* fn, MemPool* pool);

#endif
