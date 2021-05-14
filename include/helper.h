#ifndef HELPER_H
#define HELPER_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  const uint8_t *start;
  size_t sz;
} SourcePosition;

SourcePosition combine_pos(SourcePosition pos1, SourcePosition pos2);
SourcePosition make_pos(const uint8_t *buf, size_t sz);

int64_t next_vn();

void log_err(const char *fmt, ...);

/* exits after printing message */
void log_err_final(const char *fmt, ...);

#define log_internal_err(fmt, ...)                                             \
  (actual_log_internal_err(fmt, __FILE__, __LINE__, __VA_ARGS__))

void actual_log_internal_err(const char *fmt, const char *file, size_t line,
                             ...);

void log_source_err(const char *fmt, const uint8_t *base, SourcePosition, ...);

typedef struct {
  uint8_t *base;
  size_t alloc;
  size_t size;
} MemPool;

void mempool_init(MemPool *pool);
void *mempool_alloc(MemPool *pool, size_t amount);
void mempool_deinit(MemPool *pool);

typedef struct {
  MemPool *pool;
  uint8_t *data;
  size_t items; /* number of items being used */
  size_t alloc; /* space allocated, in items */
  size_t it_sz; /* size of items in bytes */
} Vector;

void vector_init(Vector *vec, size_t it_sz, MemPool *pool);
void vector_init_size(Vector *vec, size_t it_sz, MemPool *pool, size_t items);
void vector_push(Vector *vec, void *data);
void *vector_idx(Vector *vec, size_t idx); /* returns NULL on out of bounds */

/* grows vector and gives a pointer to the uninitialized data */
void *vector_alloc(Vector *vec);

#endif
