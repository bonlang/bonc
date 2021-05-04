project('comp', 'c')

src = [
  'src/helper.c',
  'src/symtable.c',
  'src/ast.c',
  'src/lexer.c',
  'src/parser.c',
  'src/sem_names.c',
  'src/sem_types.c',
  'src/comp.c',
]

inc = include_directories('include')

comp = executable(
  'comp',
  src,
  c_args : ['-Wextra', '-Werror', '-g', '-std=c99', '-pedantic'],
  include_directories : [inc]
)
