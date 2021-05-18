#ifndef SPILL_H
#define SPILL_H

#include "ssa.h"
#include "platforms.h"

void spill_prog(SSA_Prog *prog, Platform *platform);

#endif