project('comp', 'c')

src = [
  'src/comp.c',
]

inc = include_directories('include')

comp = executable(
  'comp',
  src,
  c_args : ['-Wextra', '-Werror', '-g', '-std=c99', '-pedantic'],
  include_directories : [inc]
)
