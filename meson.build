project('cydbg', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++20'])

cmake = import('cmake')

subproj = cmake.subproject('LIEF')
lief = subproj.dependency('LIB_LIEF')

exe = executable('cydbg', 'main.cpp', 'util.cpp', 'dbg.cpp', dependencies: [lief])
