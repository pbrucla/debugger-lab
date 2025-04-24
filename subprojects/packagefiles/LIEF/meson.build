project('LIEF', 'cpp',
  meson_version: '>=0.56.0'
)

ext = import('unstable-external_project')
proj = ext.add_project('cmake_wrapper.sh',
  configure_options: ['-DLIEF_C_API=OFF',
                      '-DLIEF_EXAMPLES=OFF',
                      '-DLIEF_USE_CCACHE=OFF',
                      '-DLIEF_LOGGING=OFF',
                      '-DLIEF_LOGGING_DEBUG=OFF',
                      '-DLIEF_ENABLE_JSON=OFF',
                      '-DLIEF_PE=OFF',
                      '-DLIEF_MACHO=OFF',
                      '-DLIEF_DEX=OFF',
                      '-DLIEF_ART=OFF',
                      '-DLIEF_OAT=OFF',
                      '-DLIEF_VDEX=OFF',
                      '-DCMAKE_INSTALL_PREFIX=@PREFIX@',
                      '-DCMAKE_INSTALL_INCLUDEDIR=@INCLUDEDIR@',
                      '-DCMAKE_INSTALL_LIBDIR=@LIBDIR@'
                     ],
  verbose: true
)

dep = proj.dependency('LIEF')
meson.override_dependency('LIEF_dep', dep)
