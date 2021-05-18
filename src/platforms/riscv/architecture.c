#include "platforms.h"

Platform platform_riscv_64 = {
    .name = "RV64",
    .word_size = 8,

    .num_register_classes = 2,
    .register_classes = (RegisterClass[]){
        {.reg_class = PLATFORM_REG_SPECIAL,
         .num_registers = 6,
         /* ABI names for registers are used here, but
          * the riscv register numbers are used as the
          * global register number */
         .registers =
             (PlatformRegister[]){
                 {.name = "zero", .num = 0},
                 {.name = "ra", .num = 1},
                 {.name = "sp", .num = 2},
                 {.name = "gp", .num = 3},
                 {.name = "tp", .num = 4},
                 /* The ABI says that register 8 is a
                  * "saved/register frame pointer", so
                  * it is marked as a special register
                  * to be safe. */
                 {.name = "fp", .num = 8},
             }},
        {.reg_class = PLATFORM_REG_GENERAL_PURPOSE,
         .num_registers = 26,
         .registers =
             (PlatformRegister[]){
                 {.name = "t0", .num = 5},   {.name = "t1", .num = 6},
                 {.name = "t2", .num = 7},   {.name = "s1", .num = 9},
                 {.name = "a0", .num = 10},  {.name = "a1", .num = 11},
                 {.name = "a2", .num = 12},  {.name = "a3", .num = 13},
                 {.name = "a4", .num = 14},  {.name = "a5", .num = 15},
                 {.name = "a6", .num = 16},  {.name = "a7", .num = 17},
                 {.name = "s2", .num = 18},  {.name = "s3", .num = 19},
                 {.name = "s4", .num = 20},  {.name = "s5", .num = 21},
                 {.name = "s6", .num = 22},  {.name = "s7", .num = 23},
                 {.name = "s8", .num = 24},  {.name = "s9", .num = 25},
                 {.name = "s10", .num = 26}, {.name = "s11", .num = 27},
                 {.name = "t3", .num = 28},  {.name = "t4", .num = 29},
                 {.name = "t5", .num = 30},  {.name = "t6", .num = 31},
             }},
    }};