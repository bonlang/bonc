project('bcc2', 'c')

src = [
  'src/helper.c',
  'src/error.c',
  'src/symtable.c',
  'src/ast.c',
  'src/args.c',
  'src/lexer.c',
  'src/parser.c',
  'src/sem_names.c',
  'src/sem_types.c',
  'src/sem_returns.c',
  'src/ssa.c',
  'src/ir_gen.c',
  'src/bcc2.c',

  'src/platforms/platforms.c',
  'src/platforms/x86_64/architecture.c',
  'src/platforms/riscv/architecture.c',
]

inc = include_directories('include')

diag_gen = find_program('scripts/gen_diags.py')

diag_h = custom_target(
  'diags.h',
  output : 'diags.h',
  input : 'scripts/diag_table.txt',
  command : [diag_gen, 'h', '@INPUT@', '@OUTPUT@'])

diag_txt = custom_target(
  'diags.txt',
  output : 'diags.txt',
  input : 'scripts/diag_table.txt',
  command : [diag_gen, 'c', '@INPUT@', '@OUTPUT@'])

bcc2 = executable(
  'bcc2',
  [src, diag_h, diag_txt],
  c_args : ['-Wextra', '-Werror', '-g', '-std=c99', '-pedantic'],
  include_directories : [inc],
)
