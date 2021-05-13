#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "helper.h"
#include "ssa.h"
#include "spill.h"

/* TODO:
    - Use architecture-specific info
      - Multiple register classes
      - Number of registers available
      - Additional register pressure induced by register constraints
    - Handle rematerialization
    - Handle parameters
    - Patch ssa
*/

#define MAX_REG 3

#define DIST_UNUSED SIZE_MAX
typedef struct LiveReg {
  RegId reg;
  InstId next_use;
} LiveReg;

typedef struct SpillInfo {
  RegId reg;
  /* instruction to insert spill before */
  InstId inst;
} SpillInfo;

typedef struct ReloadInfo {
  RegId reg;
  /* instruction to insert reload before */
  InstId inst;
} ReloadInfo;

typedef struct Spiller {
  MemPool pool;

  /**
   * These sets represent the mapping of virtual registers to physical
   * registers currently live. The length of either set must not exceed the
   * number of currently allocatable registers.
   */
  Vector set_working; /* LiveReg */
  Vector set_new;     /* LiveReg */

  /* TODO: Could put both in one array to make the structure more suitable to
   * a stack allocator. Would also be beneficial during SSA patching since it
   * would end up being one sorted list of nodes to insert. */
  Vector spills;  /* Spill */
  Vector reloads; /* Reload */

  SSA_Fn *fn;
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
set_insert(MemPool *pool, Vector *set, RegId reg) {
  if (set_find(set, reg) != NULL)
    return;
  if (set->items == MAX_REG)
    log_internal_err("inserting reg %%%zu into working set would cause the"
                     " length to exceeed the number of physical registers",
                     reg);

  LiveReg use = {reg, 0};
  vector_push(set, &use, pool);
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

/* Ensures that all of the values in set_new are live, and won't get spilled. */
static void
ensure_loaded(Spiller *s, InstId inst_id) {
  for (size_t i = 0; i < s->set_new.items; i++) {
    LiveReg *new_reg = vector_idx(&s->set_new, i);
    LiveReg *working_reg = set_find(&s->set_working, new_reg->reg);
    /* Remove values from the working set that exist in set_new to prevent them
     * from getting spilled */
    if (working_reg != NULL) {
      /* TODO: we could avoid searching the array again by converting the
       * from set_find to an index. */
      set_remove(&s->set_working, working_reg->reg);
    } else {
      ReloadInfo reload = {.reg = new_reg->reg, .inst = inst_id};
      vector_push(&s->reloads, &reload, &s->pool);
    }
  }
}

static void
calculate_distance(LiveReg *use, SSA_BBlock *block, InstId inst_id) {
  if (use->next_use >= inst_id)
    return;

  for (InstId id = inst_id; id < block->insts.items; id++) {
    SSA_Inst *inst = vector_idx(&block->insts, id);
    if (inst->t != INST_IMM) {
      for (uint8_t i = 0; i < inst_arity_tbl[inst->t]; i++) {
        if (inst->data.operands[i] == use->reg) {
          use->next_use = inst_id;
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
      calculate_distance(vector_idx(&s->set_working, i), block, inst_id);
    }

    qsort(s->set_working.data, s->set_working.items, s->set_working.it_sz,
          distance_compare);

    for (size_t i = 0; i < spills; i++) {
      LiveReg *use = vector_idx(&s->set_working, s->set_working.items - i - 1);
      /* We only need to generate a spill if it is used again */
      if (use->next_use != DIST_UNUSED) {
        SpillInfo spill = {.reg = use->reg, .inst = inst_id};
        vector_push(&s->spills, &spill, &s->pool);
      }
    }

    s->set_working.items -= spills;
  }

  for (size_t i = 0; i < s->set_new.items; i++) {
    LiveReg *use = vector_idx(&s->set_new, i);
    set_insert(&s->pool, &s->set_working, use->reg);
  }
}

static void
spill_bblock(Spiller *s, SSA_BBlock *block) {
  s->set_working.items = 0;
  s->spills.items = 0;
  s->reloads.items = 0;

  for (InstId i = 0; i < block->insts.items; i++) {
    SSA_Inst *inst = vector_idx(&block->insts, i);

    /* Handle inputs to instruction */
    s->set_new.items = 0;
    if (inst->t != INST_IMM) {
      for (uint8_t i = 0; i < inst_arity_tbl[inst->t]; i++) {
        set_insert(&s->pool, &s->set_new, inst->data.operands[i]);
      }
      ensure_loaded(s, i);
      combine_worksets(s, block, i);
    }

    /* Handle outputs of instruction */
    s->set_new.items = 0;
    if (inst_returns_tbl[inst->t]) {
      set_insert(&s->pool, &s->set_new, inst->result);
      combine_worksets(s, block, i);
    }
  }
}

void
spill_prog(SSA_Prog *prog) {
  Spiller spiller;
  mempool_init(&spiller.pool);
  vector_init(&spiller.set_working, sizeof(LiveReg), &spiller.pool);
  vector_init(&spiller.set_new, sizeof(LiveReg), &spiller.pool);
  vector_init(&spiller.spills, sizeof(SpillInfo), &spiller.pool);
  vector_init(&spiller.reloads, sizeof(ReloadInfo), &spiller.pool);

  for (size_t i = 0; i < prog->fns.items; i++) {

    spiller.fn = vector_idx(&prog->fns, i);
    spill_bblock(&spiller, spiller.fn->entry);
  }
}