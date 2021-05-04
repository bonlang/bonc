#ifndef HELPER_H
#define HELPER_H

#include <stddef.h>
#include <stdint.h>

void log_err(const char* fmt, ...);

/* exits after printing message */
void log_err_final(const char* fmt, ...);

void log_internal_err(const char* fmt, ...);

void log_source_err(const char* fmt, const uint8_t* base,
                    const uint8_t* location, ...);

typedef struct {
  uint8_t* base;
  size_t alloc;
  size_t size;
} MemPool;

void mempool_init(MemPool* pool);
void* mempool_alloc(MemPool* pool, size_t amount);
void mempool_deinit(MemPool* pool);

typedef struct {
  uint8_t* data;
  size_t items; /* number of items being used */
  size_t alloc; /* space allocated, in items */
  size_t it_sz; /* size of items in bytes */
} Vector;

void vector_init(Vector* vec, size_t it_sz, MemPool* pool);
void vector_push(Vector* vec, void* data, MemPool* pool);
void* vector_idx(Vector* vec, size_t idx); /* returns NULL on out of bounds */

/* grows vector and gives a pointer to the uninitialized data */
void* vector_alloc(Vector* vec, MemPool* pool);
#endif
