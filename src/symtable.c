#include "symtable.h"

#include <string.h>

#define INIT_BUCKETS 32

#define MIN(a, b) (a > b ? b : a)

Scope* scope_init(MemPool* pool, Scope* up) {
  Scope* scope = mempool_alloc(pool, sizeof(Scope));
  scope->nbuckets = INIT_BUCKETS;
  scope->up = up;
  scope->buckets = mempool_alloc(pool, sizeof(ScopeEntry*) * scope->nbuckets);
  memset(scope->buckets, 0, sizeof(ScopeEntry*) * scope->nbuckets);
  return scope;
}

static inline size_t hash_str(const uint8_t* name, size_t sz) {
  size_t total = 0;
  for (size_t i = 0; i < sz; i++) {
    total += name[i];
  }
  return total;
}

ScopeEntry* scope_insert(MemPool* pool, Scope* scope, const uint8_t* name,
                         size_t sz, VarInfo inf) {
  size_t idx = hash_str(name, sz) % scope->nbuckets;
  for (ScopeEntry* iter = scope->buckets[idx]; iter != NULL;
       iter = iter->next) {
    if (memcmp(name, iter->name, MIN(sz, iter->sz)) == 0) {
      return NULL;
    }
  }

  ScopeEntry* new_entry = mempool_alloc(pool, sizeof(ScopeEntry));
  new_entry->next = scope->buckets[idx];
  new_entry->name = name;
  new_entry->sz = sz;
  new_entry->inf = inf;
  scope->buckets[idx] = new_entry;
  return new_entry;
}

ScopeEntry* scope_find(Scope* scope, const uint8_t* name, size_t sz) {
  size_t idx = hash_str(name, sz) % scope->nbuckets;
  for (ScopeEntry* iter = scope->buckets[idx]; iter != NULL;
       iter = iter->next) {
    if (memcmp(name, iter->name, MIN(sz, iter->sz)) == 0) {
      return iter;
    }
  }
  return NULL;
}
