project('cydbg', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++20'])

capstone_dep = dependency('capstone', required: true)
rl_dep = dependency('readline', version: '>=8.2')
exe = executable('cydbg', 'main.cpp', 'util.cpp', 'dbg.cpp',
                 'operation.cpp', 'elf.cpp', 'dwarf.cpp', 
                 dependencies: [capstone_dep, rl_dep])
