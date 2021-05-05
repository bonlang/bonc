#include "symtable.h"

#include <string.h>

#define INIT_BUCKETS 32

#define MIN(a, b) (a > b ? b : a)

VarInfo make_var_info(int mut, struct Type* type) {
  VarInfo inf = {.mut = mut, .type = type, .id = 0};
  return inf;
}

Scope* scope_init(MemPool* pool, Scope* up) {
  Scope* scope = mempool_alloc(pool, sizeof(Scope));
  scope->nbuckets = INIT_BUCKETS;
  scope->up = up;
  scope->buckets = mempool_alloc(pool, sizeof(ScopeEntry*) * scope->nbuckets);
  memset(scope->buckets, 0, sizeof(ScopeEntry*) * scope->nbuckets);
  return scope;
}

static inline size_t hash_str(SourcePosition pos) {
  size_t total = 0;
  for (size_t i = 0; i < pos.sz; i++) {
    total += pos.start[i];
  }
  return total;
}

ScopeEntry* scope_insert(MemPool* pool, Scope* scope, SourcePosition pos,
                         VarInfo inf) {
  size_t idx = hash_str(pos) % scope->nbuckets;
  for (ScopeEntry* iter = scope->buckets[idx]; iter != NULL;
       iter = iter->next) {
    if (memcmp(pos.start, iter->pos.start, MIN(pos.sz, iter->pos.sz)) == 0) {
      return NULL;
    }
  }

  ScopeEntry* new_entry = mempool_alloc(pool, sizeof(ScopeEntry));
  new_entry->next = scope->buckets[idx];
  new_entry->pos = pos;
  new_entry->inf = inf;
  scope->buckets[idx] = new_entry;
  return new_entry;
}

ScopeEntry* scope_find(Scope* scope, SourcePosition pos) {
  size_t idx = hash_str(pos) % scope->nbuckets;
  for (ScopeEntry* iter = scope->buckets[idx]; iter != NULL;
       iter = iter->next) {
    if (memcmp(pos.start, iter->pos.start, MIN(pos.sz, iter->pos.sz)) == 0) {
      return iter;
    }
  }
  return NULL;
}
