project('offgl', 'c', 'cpp', default_options: ['c_std=c23', 'cpp_std=c++23', 'b_lto=true'])
args_base = ['-pipe']
if get_option('buildtype').startswith('debug')
  args = args_base + ['-DDEBUG']
  add_project_arguments(args, language: 'cpp')
  add_project_link_arguments(args, language: 'cpp')
else
  args = args_base + ['-DNDEBUG','-O3', '-march=native']
  add_project_arguments(args, language: 'cpp')
  add_project_link_arguments(args, language: 'cpp')
endif

cpp = meson.get_compiler('cpp')
inc = include_directories('glad/include')

libs = {
  'glad': static_library('glad', 'glad/src/glad.c', include_directories: inc)
}
deps = {
  # internal
  'glad': declare_dependency(link_with: libs['glad'], include_directories: inc),

  # external
  'osmesa': dependency('osmesa'),
  'libpng': dependency('libpng'),
  'zlib': dependency('zlib')
}

deps_first = [deps['osmesa'], deps['libpng'], deps['zlib'], deps['glad']]

executable('start', 'start.cpp', include_directories: inc, dependencies: deps_first)
