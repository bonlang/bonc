#ifndef TYPE_H
#define TYPE_H

enum {
  /* statically allocated types */
  TYPE_U8,
  TYPE_U16,
  TYPE_U32,
  TYPE_U64,
  TYPE_I8,
  TYPE_I16,
  TYPE_I32,
  TYPE_I64,
  TYPE_BOOL,
  TYPE_VOID,
  /* dynamically allocated types */
  TYPE_FN,
};

typedef struct Type {
  int t;
  union {
    struct {
      struct Type* ret;
      Vector args; /* Type* */
    } fn;
  } data;
} Type;

extern Type U8_const;
extern Type U16_const;
extern Type U32_const;
extern Type U64_const;
extern Type I8_const;
extern Type I16_const;
extern Type I32_const;
extern Type I64_const;
extern Type bool_const;
extern Type void_const;

int is_unsigned(int t);
int is_signed(int t);
#endif
