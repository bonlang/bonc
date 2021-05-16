#include "platforms.h"

Platform platform_x86_64_sysv = {.name = "x86_64-sysv",
                                 .word_size = 8,

                                 .num_register_classes = 2,
                                 .register_classes = (RegisterClass[]){
                                     {.reg_class = PLATFORM_REG_GENERAL_PURPOSE,
                                      .num_registers = 16,
                                      .registers =
                                          (PlatformRegister[]){
                                              {.name = "rax", .num = 0},
                                              {.name = "rcx", .num = 1},
                                              {.name = "rdx", .num = 2},
                                              {.name = "rsi", .num = 3},
                                              {.name = "rdi", .num = 4},
                                              {.name = "rbx", .num = 5},
                                              {.name = "r8", .num = 6},
                                              {.name = "r9", .num = 7},
                                              {.name = "r10", .num = 8},
                                              {.name = "r11", .num = 9},
                                              {.name = "r12", .num = 10},
                                              {.name = "r13", .num = 11},
                                              {.name = "r14", .num = 12},
                                              {.name = "r15", .num = 13},
                                          }},
                                     {.reg_class = PLATFORM_REG_SPECIAL,
                                      .num_registers = 2,
                                      .registers =
                                          (PlatformRegister[]){
                                              {.name = "bp", .num = 14},
                                              {.name = "sp", .num = 15},
                                          }},
                                 }};
