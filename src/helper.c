#define _GNU_SOURCE
#include "helper.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define RED "\033[0;31m"
#define BLUE "\033[0;34m"
#define RESET "\033[0m"

#define POOL_MAX_SZ 16777216
#define POOL_CHUNK_SZ 2048

void log_err(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, RED "error" RESET ": ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
}

void log_err_final(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, RED "error" RESET ": ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}

void log_internal_err(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, BLUE "internal error" RESET ": ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}

void mempool_init(MemPool* pool) {
  pool->base =
      mmap(NULL, POOL_MAX_SZ, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (pool->base == MAP_FAILED) {
    log_internal_err("unable to open mmap pool");
  }
  if (mprotect(pool->base, POOL_CHUNK_SZ, PROT_READ | PROT_WRITE) == -1) {
    log_internal_err("unable to block out memory in pool");
  }
  pool->size = 0;
  pool->alloc = POOL_CHUNK_SZ;
}

void mempool_deinit(MemPool* pool) {
  if (munmap(pool->base, POOL_MAX_SZ) == -1) {
    log_internal_err("unable to close memory pool");
  }
}

static inline size_t size_needed(size_t requested) {
  size_t ret = 0;
  while (ret < requested) {
    ret += POOL_CHUNK_SZ;
  }
  return ret;
}

void* mempool_alloc(MemPool* pool, size_t amount) {
  if (pool->size + amount >= pool->alloc) {
    size_t needed = size_needed(amount);
    mprotect(pool->base + pool->alloc, needed, PROT_READ | PROT_WRITE);
    pool->alloc += needed;
  }
  void* ret = pool->base + pool->size;
  pool->size += amount;
  return ret;
}
