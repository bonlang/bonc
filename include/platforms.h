#ifndef PLATFORMS_H
#define PLATFORMS_H

#include <stdint.h>
#include <stddef.h>

#include "ssa.h"

typedef struct PlatformRegister {
  const char *name;
  /* Global register number */
  size_t num;
} PlatformRegister;

enum {
  PLATFORM_REG_GENERAL_PURPOSE,
  PLATFORM_REG_SPECIAL,
};

typedef struct RegisterClass {
  /* Array of registers sorted by id */
  PlatformRegister *registers;
  size_t num_registers;

  int reg_class;
} RegisterClass;

typedef struct Platform {
  /* Display name */
  const char *name;

  RegisterClass *register_classes;
  size_t num_register_classes;

  SizeKind word_size;
} Platform;

/* Null terminated array of available platforms */
extern Platform *platforms[];

extern Platform platform_x86_64_sysv;
extern Platform platform_riscv_64;

#endif