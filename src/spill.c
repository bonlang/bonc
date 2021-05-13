#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "helper.h"
#include "ssa.h"
#include "spill.h"

/**
 * Spiller reduces *possible* register pressure to the number of available
 * registers. The implementation uses Belady's algorithm and spills the
 * registers whose next use the furthest in the future when register
 * pressure exceeds the number of registers.
 *
 * The actual register pressure does not always equal the real register
 * pressure due to ABI/architecture register constraints. There may be a way
 * to solve this cleanly that I am not aware of, but the disparity between the
 * two should be minor in nearly all cases on x86 and not an issue at all on
 * RISC architectures where there are typically no register constraints.
 *
 * The spiller may also produce subpar results on architectures that support
 * register-memory operands, though this depends on the implementation of the
 * register allocator and coalescer.
 *
 *
 * TODO:
 *  - Use architecture-specific info
 *    - Multiple register classes
 *    - Number of registers available
 *    - Additional register pressure induced by register constraints
 *    - Handle register immediate operands
 *  - Handle rematerialization
 *  - Handle parameters
 *  - Export functions from irgen related to creating new nodes/registers
 *  - Supply operands to alloca properly, we can't do this at the moment since
 *    it is a pain to create a new source position, and it would be more
 *    efficient to just have the SSA store literals for immediates
 *  - Get alloca size from the architecture description
 * Later when control flow is implemented we will need the following:
 *  - Global next use algorithm
 *  - Avoid spilling/reloading in loops
 *
 * BUG: SSA nodes are currently allocated with the spiller mempool, they need
 *      to be allocated with the mempool SSA was original allocated with since
 *      the SSA outlives the spiller mempool
 */

#define MAX_REG 3

#define DIST_UNUSED SIZE_MAX
typedef struct LiveReg {
  RegId reg;
  InstId next_use;
} LiveReg;

typedef struct SpilledReg {
  RegId reg;
  InstId result_of;
  /* Register that hold the address to the stack allocation */
  RegId address;
} SpilledReg;

typedef struct Reload {
  SpilledReg *reg;
  InstId inst;
} Reload;

typedef struct Spiller {
  MemPool pool;

  /**
   * These sets represent the mapping of virtual registers to physical
   * registers currently live. The length of either set must not exceed the
   * number of currently allocatable registers.
   */
  Vector set_working; /* LiveReg */
  Vector set_new;     /* LiveReg */

  Vector spilled_regs; /* SpilledReg */

  /* Pool of reloads for all registers, sorted by time */
  Vector reloads; /* Reload */

  SSA_Fn *fn;
  SSA_Prog *prog;
} Spiller;

/* Searches a set for a register, returns NULL if not found */
static LiveReg *
set_find(Vector *set, RegId reg) {
  for (size_t i = 0; i < set->items; i++) {
    LiveReg *use = vector_idx(set, i);
    if (use->reg == reg)
      return use;
  }
  return NULL;
}

static void
set_insert(Vector *set, RegId reg) {
  if (set_find(set, reg) != NULL)
    return;
  if (set->items == MAX_REG)
    log_internal_err("inserting reg %%%zu into working set would cause the"
                     " length to exceeed the number of physical registers",
                     reg);

  LiveReg use = {reg, 0};
  vector_push(set, &use);
}

static void
set_remove(Vector *set, RegId reg) {
  for (size_t i = 0; i < set->items; i++) {
    LiveReg *use = vector_idx(set, i);
    if (use->reg == reg) {
      vector_remove(set, i);
      break;
    }
  }
}

static void
spill_reg(Spiller *s, RegId reg, InstId result_of) {
  /* Check if already spilled */
  for (size_t i = 0; i < s->spilled_regs.items; i++) {
    SpilledReg *sr = vector_idx(&s->spilled_regs, i);
    if (sr->reg == reg)
      return;
  }

  SpilledReg sr = {.reg = reg, .result_of = result_of};
  vector_push(&s->spilled_regs, &sr);
}

static void
reload_reg(Spiller *s, RegId reg, InstId dependent) {
  for (size_t i = 0; i < s->spilled_regs.items; i++) {
    SpilledReg *sr = vector_idx(&s->spilled_regs, i);
    if (sr->reg == reg) {
      Reload reload = {.reg = sr, .inst = dependent};
      vector_push(&s->reloads, &reload);
      return;
    }
  }

  log_internal_err("tried to load %%%zu which has not yet been spilled", reg);
}

/* Ensures that all of the values in set_new are live, and won't get spilled. */
static void
ensure_loaded(Spiller *s, InstId inst_id) {
  for (size_t i = 0; i < s->set_new.items; i++) {
    LiveReg *new_reg = vector_idx(&s->set_new, i);
    LiveReg *working_reg = set_find(&s->set_working, new_reg->reg);
    /* Remove values from the working set that exist in set_new to prevent them
     * from getting spilled */
    if (working_reg != NULL) {
      set_remove(&s->set_working, working_reg->reg);
    } else {
      /* BUG: reloads should not be inserted before spills */
      reload_reg(s, new_reg->reg, inst_id);
    }
  }
}

static void
calculate_next_use(LiveReg *use, SSA_BBlock *block, InstId inst_id) {
  if (use->next_use >= inst_id)
    return;

  for (InstId id = inst_id; id < block->insts.items; id++) {
    SSA_Inst *inst = vector_idx(&block->insts, id);
    if (inst->t != INST_IMM) {
      for (uint8_t i = 0; i < inst_arity_tbl[inst->t]; i++) {
        if (inst->data.operands[i] == use->reg) {
          use->next_use = id;
          return;
        }
      }
    }
  }

  use->next_use = DIST_UNUSED;
}

static int
distance_compare(const void *a, const void *b) {
  const InstId l = ((LiveReg *)a)->next_use;
  const InstId r = ((LiveReg *)b)->next_use;
  if (l > r)
    return 1;
  else if (l < r)
    return -1;
  else
    return 0;
}

/**
 * Adds the values of set_new into set_working, and inserts spills to ensure
 * that the set length does not exceed the maximum number of registers.
 */
static void
combine_worksets(Spiller *s, SSA_BBlock *block, InstId inst_id) {
  /* Make enough space in set_working to concatenate set_new */
  if (s->set_working.items + s->set_new.items > MAX_REG) {
    size_t spills = s->set_working.items + s->set_new.items - MAX_REG;

    for (size_t i = 0; i < s->set_working.items; i++) {
      calculate_next_use(vector_idx(&s->set_working, i), block, inst_id);
    }

    qsort(s->set_working.data, s->set_working.items, s->set_working.it_sz,
          distance_compare);

    for (size_t i = 0; i < spills; i++) {
      LiveReg *use = vector_idx(&s->set_working, s->set_working.items - i - 1);
      /* We only need to generate a spill if it is used again */
      if (use->next_use != DIST_UNUSED) {
        spill_reg(s, use->reg, inst_id);
      }
    }

    s->set_working.items -= spills;
  }

  for (size_t i = 0; i < s->set_new.items; i++) {
    LiveReg *use = vector_idx(&s->set_new, i);
    set_insert(&s->set_working, use->reg);
  }
}

static void
spill_bblock(Spiller *s, SSA_BBlock *block) {
  s->set_working.items = 0;
  s->spilled_regs.items = 0;
  s->reloads.items = 0;

  /* Generate spills and reloads for all instructions in the current block*/
  for (InstId i = 0; i < block->insts.items; i++) {
    SSA_Inst *inst = vector_idx(&block->insts, i);

    /* Handle inputs to instruction */
    s->set_new.items = 0;
    if (inst->t != INST_IMM) {
      for (uint8_t i = 0; i < inst_arity_tbl[inst->t]; i++) {
        set_insert(&s->set_new, inst->data.operands[i]);
      }
      ensure_loaded(s, i);
      combine_worksets(s, block, i);
    }

    /* Handle outputs of instruction */
    s->set_new.items = 0;
    if (inst_returns_tbl[inst->t]) {
      set_insert(&s->set_new, inst->result);
      combine_worksets(s, block, i);
    }
  }
}

static void
spills_allocate(Spiller *s, SSA_BBlock *block) {
  for (size_t i = 0; i < s->spilled_regs.items; i++) {
    SpilledReg *sr = vector_idx(&s->spilled_regs, i);

    SSA_Reg *target_reginfo = vector_idx(&s->fn->regs, sr->reg);
    SSA_Reg *alloca_reginfo = vector_alloc(&s->fn->regs);
    alloca_reginfo->sz = target_reginfo->sz;
    sr->address = s->fn->regs.items;

    SSA_Inst alloca = {.t = INST_ALLOCA, .sz = SZ_64, .result = sr->address};
    vector_insert(&block->insts, 0, &alloca);
  }
}

static void
spills_insert_loads(Spiller *s, SSA_BBlock *block) {
  size_t offset = s->spilled_regs.items;
  for (size_t i = 0; i < s->reloads.items; i++) {
    Reload *reload = vector_idx(&s->reloads, i);
    SSA_Inst *target_inst = vector_idx(&block->insts, reload->inst + offset);
    SSA_Inst sr_inst = {.sz = target_inst->sz, .result = 0};

    SSA_Reg *reginfo = vector_alloc(&s->fn->regs);
    reginfo->sz = target_inst->sz;
    sr_inst.result = s->fn->regs.items;
    sr_inst.t = INST_LOAD;
    sr_inst.data.operands[0] = reload->reg->address;

    vector_insert(&block->insts, reload->inst + offset, &sr_inst);
    offset++;

    /* Find the next load for this register */
    InstId next_load = block->insts.items;
    for (size_t j = i + 1; j < s->reloads.items; j++) {
      Reload *r = vector_idx(&s->reloads, j);
      if (r->reg == reload->reg) {
        next_load = reload->inst + offset;
      }
    }

    /* Patch future uses of the old register up to the next reload */
    for (size_t j = reload->inst + offset; j < next_load; j++) {
      SSA_Inst *inst = vector_idx(&block->insts, j);
      if (inst->t != INST_IMM) {
        for (uint8_t i = 0; i < inst_arity_tbl[inst->t]; i++) {
          if (inst->data.operands[i] == reload->reg->reg) {
            inst->data.operands[i] = sr_inst.result;
          }
        }
      }
    }
  }
}

static void
spills_insert_stores(Spiller *s, SSA_BBlock *block) {
  for (size_t i = 0; i < s->spilled_regs.items; i++) {
    SpilledReg *sr = vector_idx(&s->spilled_regs, i);
    for (size_t j = 0; j < block->insts.items; j++) {
      SSA_Inst *inst = vector_idx(&block->insts, j);
      if (inst->result == sr->reg) {
        SSA_Reg *alloca_reginfo = vector_idx(&s->fn->regs, sr->address);
        SSA_Inst store = {.t = INST_STORE,
                          .sz = alloca_reginfo->sz,
                          .data.operands = {sr->address, sr->reg}};
        vector_insert(&block->insts, j + 1, &store);
        break;
      }
    }
  }
}

void
spill_prog(SSA_Prog *prog) {
  Spiller spiller;
  mempool_init(&spiller.pool);
  vector_init(&spiller.set_working, sizeof(LiveReg), &spiller.pool);
  vector_init(&spiller.set_new, sizeof(LiveReg), &spiller.pool);
  vector_init(&spiller.spilled_regs, sizeof(SpilledReg), &spiller.pool);
  vector_init(&spiller.reloads, sizeof(Reload), &spiller.pool);

  for (size_t i = 0; i < prog->fns.items; i++) {
    spiller.prog = prog;
    spiller.fn = vector_idx(&prog->fns, i);
    spill_bblock(&spiller, spiller.fn->entry);
    spills_allocate(&spiller, spiller.fn->entry);
    spills_insert_loads(&spiller, spiller.fn->entry);
    spills_insert_stores(&spiller, spiller.fn->entry);
  }
}