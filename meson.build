project('bonc', 'c')

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
  'src/bonc.c',

  'src/platforms/platforms.c',
  'src/platforms/x86_64/architecture.c',
  'src/platforms/riscv/architecture.c',
]

inc = include_directories('include')

bonc = executable(
  'bonc',
  src,
  c_args : ['-Wextra', '-Werror', '-g', '-std=c99', '-pedantic'],
  include_directories : [inc]
)
