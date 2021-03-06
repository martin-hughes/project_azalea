import os
import platform

import build_support.dependencies as dependencies

def main_build_script(linux_build, config_env):
  # Most components and options are excluded from the Windows build. Only the test program builds on Windows.
  if linux_build:
    paths = path_builder(config_env)

    # API headers
    kernel_env = build_default_env(linux_build)
    headers = kernel_env.File(Glob("kernel/user_interfaces/*"))
    user_headers = kernel_env.File(Glob("user/libs/libazalea/azalea/*.h"))

    ui_folder = os.path.join(paths.kernel_headers_folder, "azalea")
    kernel_env.Install(ui_folder, headers)
    kernel_env.Install(ui_folder, user_headers)

    main_compile_flags = [ # Flags for all C-like builds
      '-Wall',
      '-nostdinc',
      '-nostdlib',
      '-nodefaultlibs',
      '-mcmodel=large',
      '-ffreestanding',
      '-fno-exceptions',
      '-funwind-tables',
      '-U _LINUX',
      '-U __linux__',
      '-D __AZALEA__',
      '-D __AZ_KERNEL__',
      '-D KL_TRACE_BY_SERIAL_PORT',

      # Uncomment this define to include a serial port based terminal as well as the normal VGA/keyboard one. This is
      # normally disabled as the locking in the kernel isn't great at the moment so it's a bit unstable.
      #'-D INC_SERIAL_TERM',
    ]

    cxx_flags = [ # Flags specific to C++ builds
      '-nostdinc++',
      '-std=c++17',
      '-D _LIBCPP_NO_EXCEPTIONS',
    ]

    # Main kernel part
    kernel_env['CXXFLAGS'] = ' '.join(main_compile_flags + cxx_flags)
    kernel_env['CFLAGS'] = ' '.join(main_compile_flags)
    kernel_env['LINKFLAGS'] = "-T build_support/kernel_stage.ld --start-group "
    kernel_env['LINK'] = 'ld -gc-sections -Map output/kernel_map.map --eh-frame-hdr'
    kernel_env['LIBPATH'] = [paths.libcxx_kernel_lib_folder,
                             paths.acpica_lib_folder,
                             paths.libc_lib_folder,
                             paths.libunwind_kernel_lib_folder,
                             paths.libcxxabi_kernel_lib_folder,
                            ]
    kernel_env['LIBS'] = [ 'acpica', 'azalea_libc_kernel', 'c++', 'thread_adapter', 'unwind', 'c++abi' ]
    kernel_env.AppendENVPath('CPATH', '#/kernel')
    kernel_env.AppendENVPath('CPATH', os.path.join(paths.libcxx_kernel_headers_folder, 'c++/v1'))
    kernel_env.AppendENVPath('CPATH', paths.acpica_headers_folder)
    kernel_env.AppendENVPath('CPATH', paths.libc_headers_folder)
    kernel_env.AppendENVPath('CPATH', paths.kernel_headers_folder)
    kernel_env.AppendENVPath('CPATH', paths.libunwind_kernel_headers_folder)
    kernel_obj = default_build_script(dependencies.kernel, "kernel64.sys", kernel_env, "kernel")
    kernel_install_obj = kernel_env.Install(paths.sys_image_root, kernel_obj)
    kernel_env.AddPostAction(kernel_obj, disasm_action)

    # User mode API and programs environment
    user_mode_env = build_default_env(linux_build)
    user_mode_env['CXXFLAGS'] = '-Wall -nostdinc -nostdinc++ -nostdlib -mcmodel=large -std=c++17 -U _LINUX -U __linux__ -D __AZALEA__'
    user_mode_env['CFLAGS'] = '-Wall -nostdinc -nostdlib -mcmodel=large -U _LINUX -U __linux__ -D __AZALEA__'
    user_mode_env['LIBPATH'] = [paths.libc_lib_folder,
                                # Uncomment to build ncurses test program.
                                #os.path.join(paths.developer_root, "ncurses", "lib"),
                                paths.libcxx_user_lib_folder,
                                paths.libcxxabi_user_lib_folder,
                                paths.libunwind_user_lib_folder,
                                paths.libcxxrt_lib_folder,
                               ]
    user_mode_env['LINK'] = 'ld -Map ${TARGET}.map --eh-frame-hdr'

    # User mode part of the API
    api_lib_env = user_mode_env.Clone()
    api_lib_env.AppendENVPath('CPATH', '#user/libs/libazalea')
    api_lib_env.AppendENVPath('CPATH', paths.libc_headers_folder)
    api_lib_env.AppendENVPath('CPATH', paths.kernel_headers_folder)
    user_api_obj = default_build_script(dependencies.user_mode_api, "azalea", api_lib_env, "api_library", False)
    api_install_obj = api_lib_env.Install(paths.kernel_lib_folder, user_api_obj)

    user_mode_env['LIBS'] = [
      #'ncurses', # Uncomment to build ncurses test program.
      'libc++',
      'libc++abi',
      'libunwind',
      'pthread',
      'libc++start',
      'azalea_linux_shim',
      'azalea_libc',
      user_api_obj,
    ]

    user_mode_env.AppendENVPath('CPATH', os.path.join(paths.libcxx_user_headers_folder, "c++/v1"))
    user_mode_env.AppendENVPath('CPATH', paths.libc_headers_folder)
    user_mode_env.AppendENVPath('CPATH', paths.kernel_headers_folder)
    user_mode_env.AppendENVPath('CPATH', os.path.join(paths.developer_root, "ncurses", "include"))

    # User mode programs
    init_deps = dependencies.init_program
    init_prog_obj = default_build_script(init_deps, "initprog", user_mode_env, "init_program")
    init_install_obj = user_mode_env.Install(paths.sys_image_root, init_prog_obj)
    user_mode_env.AddPostAction(init_prog_obj, disasm_action)

    shell_deps = dependencies.shell_program
    shell_prog_obj = default_build_script(shell_deps, "shell", user_mode_env, "simple_shell")
    shell_install_obj = user_mode_env.Install(paths.sys_image_root, shell_prog_obj)

    echo_deps = dependencies.echo_program
    echo_prog_obj = default_build_script(echo_deps, "echo", user_mode_env, "echo_prog")
    echo_install_obj = user_mode_env.Install(paths.sys_image_root, echo_prog_obj)

    list_deps = dependencies.list_program
    list_prog_obj = default_build_script(list_deps, "list", user_mode_env, "list_prog")
    list_install_obj = user_mode_env.Install(paths.sys_image_root, list_prog_obj)

    # Uncomment to build ncurses test program
    #ncurses_deps = dependencies.ncurses_program
    #ncurses_prog_obj = default_build_script(ncurses_deps, "ncurses", user_mode_env, "ncurses_prog")
    #ncurses_install_obj = user_mode_env.Install(paths.sys_image_root, ncurses_prog_obj)

    ot_deps = dependencies.online_tests
    ot_prog_obj = default_build_script(ot_deps, "online_tests", user_mode_env, "ot_prog")
    ot_install_obj = user_mode_env.Install(paths.sys_image_root, ot_prog_obj)

    # Install and other simple targets
    kernel_env.Alias('install-headers', ui_folder)
    Default(kernel_install_obj)
    Default(init_install_obj)
    Default(shell_install_obj)
    Default(echo_install_obj)
    Default(list_install_obj)
    #Default(ncurses_install_obj)
    Default(api_install_obj)
    Default(ot_install_obj)

  # Unit test program
  test_script_env = build_default_env(linux_build)

  additional_defines = ' -D AZALEA_TEST_CODE -D KL_TRACE_BY_STDOUT'

  if linux_build:
    test_script_env['LINKFLAGS'] = '-L/usr/lib/llvm-6.0/lib/clang/6.0.0/lib/linux -Wl,--start-group'
    cxx_flags = '-g -O0 -std=c++17 -Wunknown-pragmas'
    test_script_env['LIBS'] = [ ]
    if config_env['test_attempt_mem_leak_check']:
      test_script_env['LIBS'].append ('clang_rt.asan-x86_64')
      cxx_flags = cxx_flags + ' -fsanitize=address'

    cxx_flags = cxx_flags + additional_defines
    exe_name = 'main-tests'
    test_script_env['LIBS'].append([ 'libvirtualdisk',
                                     'pthread',
                                     'stdc++fs',
                                   ])
    additional_include_tag = 'CPATH'
  else:
    additional_defines += ' -D _DEBUG /MTd'
    if  config_env['test_attempt_mem_leak_check']:
      additional_defines += ' -D UT_MEM_LEAK_CHECK'
    test_script_env['LINKFLAGS'] = '/DEBUG:FULL /MAP:output\\main-tests.map /INCREMENTAL /NOLOGO'
    cxx_flags = additional_defines + ' /nologo /EHac /Od /ZI /Fdoutput\\main-tests.pdb /std:c++17 /Zc:__cplusplus /permissive-'
    exe_name = 'main-tests.exe'
    additional_include_tag = 'INCLUDE'

  test_script_env['CXXFLAGS'] = cxx_flags
  test_script_env.AppendENVPath(additional_include_tag, '#/external/googletest/googletest/include')
  test_script_env.AppendENVPath(additional_include_tag, '#/kernel')
  test_script_env.AppendENVPath(additional_include_tag, '#/external/googletest/googletest/')
  tests_obj = default_build_script(dependencies.main_tests, exe_name, test_script_env, "main_tests")
  Default(tests_obj)

  if not linux_build:
    # Remove the idb, pdb, map and ilk files when doing a clean.
    test_script_env.SideEffect("output\\main-tests.ilk", tests_obj)
    test_script_env.SideEffect("output\\main-tests.idb", tests_obj)
    test_script_env.SideEffect("output\\main-tests.pdb", tests_obj)
    test_script_env.SideEffect("output\\main-tests.mapS", tests_obj)
    test_script_env.Clean(tests_obj, "output\\main-tests.ilk")
    test_script_env.Clean(tests_obj, "output\\main-tests.idb")
    test_script_env.Clean(tests_obj, "output\\main-tests.pdb")
    test_script_env.Clean(tests_obj, "output\\main-tests.map")


def copy_environ_param(name, env_dict):
  if name in os.environ:
    env_dict[name] = os.environ[name]

def build_default_env(linux_build):
  env_to_copy_in = { }
  copy_environ_param('PATH', env_to_copy_in)
  copy_environ_param('CPATH', env_to_copy_in)
  copy_environ_param('TEMP', env_to_copy_in)
  copy_environ_param('TMP', env_to_copy_in)
  copy_environ_param('INCLUDE', env_to_copy_in)
  copy_environ_param('LIB', env_to_copy_in)

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

  if linux_build:
    env['CC'] = 'clang'
    env['CXX'] = 'clang++'
    env['LINK'] = 'clang++'
    env['ASFLAGS'] = '-f elf64'
  else:
    env['ASFLAGS'] = '-f win64'

  return env

def default_build_script(deps, output_name, env, part_name, is_program = True):
  # Transform the name of dependencies so that the intermediate files end up in output/(part name)/... and remove the
  # SConscript part of the path.
  dependencies_out = [ ]
  for dep in deps:
    dep_out = dep
    if isinstance(dep, str) or isinstance(dep, basestring):
      out_dir = os.path.join('output', part_name)
      out_dir = os.path.join(out_dir, dep.strip('#'))
      out_dir = os.path.dirname(out_dir)
      dep_out = env.SConscript(dep, 'env', variant_dir=out_dir, duplicate=0)
    dependencies_out.append(dep_out)

  if is_program:
    return env.Program(os.path.join('output', output_name), dependencies_out)
  else:
    return env.StaticLibrary(os.path.join('output', output_name), dependencies_out)

def disassemble_cmd(target, source, env):
  disasm_cmd = 'ndisasm -b64 -a -p intel {obj_file} > {disassembly}'
  output_file = str(target[0]) + '.asm'
  os.system (disasm_cmd.format(obj_file = str(target[0]), disassembly = output_file))

  return None

def construct_variables(linux_build):
  var = Variables(["build_support/default_config.py", "variables.cache" ], ARGUMENTS)
  if linux_build:
    var.AddVariables(
      PathVariable("sys_image_root",
                   "Root of Azalea system image. Look for include files, libraries and installation locations here.",
                   None,
                   PathVariable.PathIsDir))
  var.AddVariables(
    BoolVariable("test_attempt_mem_leak_check",
                 "Should the test scripts attempt memory leak detection?",
                 False))

  e = Environment(variables = var)

  var.Save("variables.cache", e)

  return e

class path_builder:
  def __init__(self, cfg_env):
    self.sys_image_root = cfg_env["sys_image_root"]
    self.developer_root = os.path.join(self.sys_image_root, "apps", "developer")
    self.kernel_headers_folder = os.path.join(self.developer_root, "kernel", "include")
    self.kernel_lib_folder = os.path.join(self.developer_root, "kernel", "lib")

    # Azalea libc
    self.libc_headers_folder = os.path.join(self.developer_root, "libc", "include")
    self.libc_lib_folder = os.path.join(self.developer_root, "libc", "lib")

    # Azalea libc++
    self.libcxx_kernel_headers_folder = os.path.join(self.developer_root, "libcxx-kernel", "include")
    self.libcxx_kernel_lib_folder = os.path.join(self.developer_root, "libcxx-kernel", "lib")
    self.libcxx_user_headers_folder = os.path.join(self.developer_root, "libcxx", "include")
    self.libcxx_user_lib_folder = os.path.join(self.developer_root, "libcxx", "lib")
    self.libcxxrt_lib_folder = os.path.join(self.developer_root, "compiler-rt", "lib")

    # Azalea libcxxabi
    self.libcxxabi_kernel_lib_folder = os.path.join(self.developer_root, "libcxxabi-kernel", "lib")
    self.libcxxabi_user_lib_folder = os.path.join(self.developer_root, "libcxxabi", "lib")

    # Azalea libunwind
    self.libunwind_kernel_headers_folder = os.path.join(self.developer_root, "libunwind-kernel", "include")
    self.libunwind_kernel_lib_folder = os.path.join(self.developer_root, "libunwind-kernel", "lib")
    self.libunwind_user_headers_folder = os.path.join(self.developer_root, "libunwind", "include")
    self.libunwind_user_lib_folder = os.path.join(self.developer_root, "libunwind", "lib")

    # Azalea ACPICA
    self.acpica_headers_folder = os.path.join(self.developer_root, "acpica", "include")
    self.acpica_lib_folder = os.path.join(self.developer_root, "acpica", "lib")

# Create actions for our custom commands.
disasm_action = Action(disassemble_cmd, "Disassembling $TARGET")

# Determine whether this is a Linux or Windows-based build.
sys_name = platform.system()

if sys_name == 'Linux':
  linux_build = True
elif sys_name == 'Windows':
  linux_build = False
  print ("Windows build - only the test program is built.")
else:
  print("Unknown build platform")
  exit(0)

config_env = construct_variables(linux_build)

main_build_script(linux_build, config_env)