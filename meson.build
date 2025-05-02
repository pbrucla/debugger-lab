project('cydbg', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++20'])

subproject('LIEF')
lief_dep = dependency('LIEF_dep')

exe = executable('cydbg', 'main.cpp', 'util.cpp', 'dbg.cpp', 'elf.cpp', dependencies: [lief_dep])
