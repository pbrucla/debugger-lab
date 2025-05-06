project('cydbg', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++20'])

capstone_dep = dependency('capstone', required: true)
exe = executable('cydbg', 'main.cpp', 'util.cpp', 'dbg.cpp', dependencies: [capstone_dep])

