#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stddef.h>
#include <stdint.h>

#include "helper.h"
#include "type.h"

typedef struct {
  int mut;
  int64_t id; /* used in IR generation */
  struct Type* type;
} VarInfo;

VarInfo make_var_info(int mut, struct Type* type);

typedef struct ScopeEntry {
  SourcePosition pos;
  struct ScopeEntry* next;
  VarInfo inf;
} ScopeEntry;

typedef struct Scope {
  struct Scope* up;

  size_t nbuckets;
  ScopeEntry** buckets;
} Scope;

Scope* scope_init(MemPool* pool, Scope* up);

/* returns NULL if already found */
ScopeEntry* scope_insert(MemPool* pool, Scope* scope, SourcePosition pos,
                         VarInfo inf);

/* returns NULL no entry */
ScopeEntry* scope_find(Scope* scope, SourcePosition pos);

#endif
