project(
  'solakon-monitor',
  'cpp',
  version: '0.1.0',
  default_options: [
    'cpp_std=c++20',
    'warning_level=3',
    'optimization=2',
  ],
)

# Compiler flags
cpp = meson.get_compiler('cpp')
deps = [
  cpp.find_library('pthread', required: true),
]

inc = include_directories('include')

sources = [
  'src/modbus_client.cpp',
  'src/solakon_device.cpp',
  'src/ui.cpp',
  'src/http_server.cpp',
  'src/config_loader.cpp',
  'src/network_scanner.cpp',
  'src/main.cpp',
]

executable(
  'solakon-monitor',
  sources,
  include_directories: inc,
  dependencies: deps,
  install: false,
  c_args: ['-std=c++20', '-Wall', '-Wextra', '-O2'],
  cpp_args: ['-std=c++20', '-Wall', '-Wextra', '-O2'],
)

# Test executable
catch2_dep = dependency('catch2-with-main')

test_sources = [
  'tests/test_main.cpp',
]

test_exe = executable(
  'solakon-monitor_test',
  test_sources,
  'src/ui.cpp',
  include_directories: inc,
  dependencies: [catch2_dep],
  c_args: ['-std=c++20', '-Wall', '-Wextra', '-O0', '-g', '-fprofile-arcs', '-ftest-coverage'],
  cpp_args: ['-std=c++20', '-Wall', '-Wextra', '-O0', '-g', '-fprofile-arcs', '-ftest-coverage'],
  link_args: ['-fprofile-arcs', '-ftest-coverage'],
)

test('solakon-monitor_test', test_exe)
