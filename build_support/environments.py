import os
from SCons.Script import *

import build_support.flags as flags

def copy_environ_param(name, env_dict):
  if name in os.environ:
    env_dict[name] = os.environ[name]

def build_default_env(clang_build):
  env_to_copy_in = { }
  copy_environ_param('PATH', env_to_copy_in)
  copy_environ_param('CPATH', env_to_copy_in)
  copy_environ_param('TEMP', env_to_copy_in)
  copy_environ_param('TMP', env_to_copy_in)
  copy_environ_param('INCLUDE', env_to_copy_in)
  copy_environ_param('LIB', env_to_copy_in)

  # For attempting builds on Windows, try something like this:
  # tools=['nasm', 'clangxx', 'clang', 'ar', 'gnulink', 'install']
  env = Environment(ENV = env_to_copy_in, tools=['default', 'nasm'])
  env['CPPPATH'] = '#'
  env['AS'] = 'nasm'
  env['RANLIBCOM'] = ''
  env['ASCOMSTR'] =   "Assembling:   $TARGET"
  env['CCCOMSTR'] =   "Compiling:    $TARGET"
  env['CXXCOMSTR'] =  "Compiling:    $TARGET"
  env['LINKCOMSTR'] = "Linking:      $TARGET"
  env['ARCOMSTR'] =   "(pre-linking) $TARGET"
  env['LIBS'] = [ ]

  if clang_build:
    env['CC'] = 'clang'
    env['CXX'] = 'clang++'
    env['LINK'] = 'clang++'
    env['ASFLAGS'] = '-f elf64'
  else:
    env['ASFLAGS'] = '-f win64'

  return env

def construct_envs(clang_build, paths):
  kernel_env = build_default_env(clang_build)

  kernel_env['CXXFLAGS'] = ' '.join(flags.kernel_compile_flags + flags.kernel_cxx_flags)
  kernel_env['CFLAGS'] = ' '.join(flags.kernel_compile_flags)
  kernel_env['LINKFLAGS'] = "-T build_support/kernel_stage.ld --start-group "
  kernel_env['LINK'] = 'ld -Map output/kernel_map.map --eh-frame-hdr' #-gc-sections
  kernel_env['PROGSUFFIX'] = 'sys'
  kernel_env.AppendENVPath('CPATH', '#/kernel/include')
  kernel_env.AppendENVPath('CPATH', os.path.join(paths.libcxx_kernel_headers_folder, 'c++/v1'))
  kernel_env.AppendENVPath('CPATH', paths.acpica_headers_folder)
  kernel_env.AppendENVPath('CPATH', paths.libc_headers_folder)
  kernel_env.AppendENVPath('CPATH', paths.kernel_headers_folder)
  kernel_env.AppendENVPath('CPATH', paths.libunwind_kernel_headers_folder)

  kernel_env['LIBPATH'] = [paths.libcxx_kernel_lib_folder,
                            paths.acpica_lib_folder,
                            paths.libc_lib_folder,
                            paths.libunwind_kernel_lib_folder,
                            paths.libcxxabi_kernel_lib_folder,
                          ]

  kernel_arch_env = kernel_env.Clone()
  kernel_arch_env.PrependENVPath('CPATH', '#/kernel/arch/x64/include')

  user_mode_env = build_default_env(clang_build)
  user_mode_env['CXXFLAGS'] = ' '.join(flags.user_compile_flags + flags.user_cxx_flags)
  user_mode_env['CFLAGS'] =   ' '.join(flags.user_compile_flags)
  user_mode_env['LIBPATH'] = [paths.libc_lib_folder,
                              # Uncomment to build ncurses test program.
                              #os.path.join(paths.developer_root, "ncurses", "lib"),
                              paths.libcxx_user_lib_folder,
                              paths.libcxxabi_user_lib_folder,
                              paths.libunwind_user_lib_folder,
                              paths.libcxxrt_lib_folder,
                              ]
  user_mode_env['LINK'] = 'ld -Map ${TARGET}.map --eh-frame-hdr'


  api_lib_env = user_mode_env.Clone()
  api_lib_env.AppendENVPath('CPATH', '#user/libs/libazalea')
  api_lib_env.AppendENVPath('CPATH', paths.libc_headers_folder)
  api_lib_env.AppendENVPath('CPATH', paths.kernel_headers_folder)


  user_mode_env.AppendENVPath('CPATH', os.path.join(paths.libcxx_user_headers_folder, "c++/v1"))
  user_mode_env.AppendENVPath('CPATH', paths.libc_headers_folder)
  user_mode_env.AppendENVPath('CPATH', paths.kernel_headers_folder)
  user_mode_env.AppendENVPath('CPATH', os.path.join(paths.developer_root, "ncurses", "include"))


  test_script_env = build_default_env(clang_build)

  return {
    "kernel" : kernel_env,
    "kernel_arch" : kernel_arch_env,
    "user" : user_mode_env,
    "api_library" : api_lib_env,
    "unit_tests" : test_script_env,
  }
