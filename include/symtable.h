#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stddef.h>
#include <stdint.h>

#include "helper.h"

typedef struct ScopeEntry {
  const uint8_t* name;
  size_t sz;
  struct ScopeEntry* next;
} ScopeEntry;

typedef struct Scope {
  struct Scope* up;

  size_t nbuckets;
  ScopeEntry** buckets;
} Scope;

void scope_init(Scope* scope, MemPool* pool, Scope* up);

/* returns NULL if already found */
ScopeEntry* scope_insert(MemPool* pool, Scope* scope, const uint8_t* name,
                         size_t sz);

/* returns NULL no entry */
ScopeEntry* scope_find(Scope* scope, const uint8_t* name, size_t sz);

#endif
