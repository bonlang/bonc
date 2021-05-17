project('bcc2', 'c')

src = [
  'src/helper.c',
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
]

inc = include_directories('include')

bcc2 = executable(
  'bcc2',
  src,
  c_args : ['-Wextra', '-Werror', '-g', '-std=c99', '-pedantic'],
  include_directories : [inc]
)
