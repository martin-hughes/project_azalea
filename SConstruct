import os
import platform

import build_support.dependencies as dependencies
import build_support.environments as envs

def main_build_script(linux_build, config_env):
  # Most components and options are excluded from the Windows build. Only the test program builds on Windows.
  paths = path_builder(config_env)
  clang_build = linux_build or not config_env["use_msvc"]

  env_dict = envs.construct_envs(clang_build, paths)

  kernel_env = env_dict["kernel"]
  kernel_arch_env = env_dict["kernel_arch"]
  user_mode_env = env_dict["user"]
  api_lib_env = env_dict["api_library"]
  test_script_env = env_dict["unit_tests"]

  if clang_build:
    # API headers
    headers = kernel_env.File(Glob("kernel/interface/azalea/*"))
    user_headers = kernel_env.File(Glob("user/libs/libazalea/azalea/*.h"))

    ui_folder = os.path.join(paths.kernel_headers_folder, "azalea")
    kernel_env.Install(ui_folder, headers)
    kernel_env.Install(ui_folder, user_headers)

    cxx_thread_header_folder = os.path.join(paths.libcxx_kernel_headers_folder, "c++", "v1")
    cxx_thread_header = kernel_env.File("kernel/interface/libcxx/__external_threading")
    kernel_env.Install(cxx_thread_header_folder, cxx_thread_header)

    # Main kernel part

    # Create a different environment for the architecture specific part, to avoid contaminating the include path for
    # the core of the kernel and to help enforce a good separation between the core and architecture specific parts.
    arch_lib = default_build_script(dependencies.x64_part, "x64-lib", kernel_arch_env, "x64-lib", None, paths.sys_image_root, True, False,  False)
    Requires(arch_lib, cxx_thread_header_folder)
    Requires(arch_lib, ui_folder)

    kernel_env['LIBS'] = [ arch_lib, 'acpica', 'azalea_libc_kernel', 'c++', 'unwind', 'c++abi' ]
    kernel_obj = default_build_script(dependencies.kernel, "kernel64.sys", kernel_env, "kernel", "kernel", paths.sys_image_root, True, True)
    kernel_env.AddPostAction(kernel_obj, disasm_action)
    Requires(kernel_obj, cxx_thread_header_folder)
    Requires(kernel_obj, ui_folder)

    # User mode part of the API
    user_api_obj = default_build_script(dependencies.user_mode_api, "azalea", api_lib_env, "api_library", "api", paths.kernel_lib_folder, True, True, False)

    # User mode programs
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

    init_deps = dependencies.init_program
    init_prog_obj = default_build_script(init_deps, "initprog", user_mode_env, "init_program", "init_prog", paths.sys_image_root, True, True)
    user_mode_env.AddPostAction(init_prog_obj, disasm_action)

    shell_deps = dependencies.shell_program
    shell_prog_obj = default_build_script(shell_deps, "shell", user_mode_env, "simple_shell", "shell", paths.sys_image_root, True, True)

    echo_deps = dependencies.echo_program
    echo_prog_obj = default_build_script(echo_deps, "echo", user_mode_env, "echo_prog", "echo", paths.sys_image_root, True, True)

    list_deps = dependencies.list_program
    list_prog_obj = default_build_script(list_deps, "list", user_mode_env, "list_prog", "list", paths.sys_image_root, True, True)

    ncurses_deps = dependencies.ncurses_program
    ncurses_prog_obj = default_build_script(ncurses_deps, "ncurses", user_mode_env, "ncurses_prog", "ncurses", paths.sys_image_root, False, False)

    ot_deps = dependencies.online_tests
    ot_prog_obj = default_build_script(ot_deps, "online_tests", user_mode_env, "ot_prog", "online_test", paths.sys_image_root, True, True)

    # Target for installing headers.
    kernel_env.Alias('install-headers', ui_folder)
    kernel_env.Alias('install-headers', cxx_thread_header_folder)
    Default('install-headers')

  # Unit test program

  additional_defines = ' -D AZALEA_TEST_CODE -D KL_TRACE_BY_STDOUT'

  if clang_build:
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
  test_script_env.AppendENVPath(additional_include_tag, '#/test/unit')
  test_script_env.AppendENVPath(additional_include_tag, '#/kernel/include')
  test_script_env.AppendENVPath(additional_include_tag, '#/kernel/interface')
  test_script_env.AppendENVPath(additional_include_tag, '#/external/googletest/googletest/')
  tests_obj = default_build_script(dependencies.main_tests, exe_name, test_script_env, "main_tests", "main_tests", True, False, None)
  #Default(tests_obj)

  if not clang_build:
    # Remove the idb, pdb, map and ilk files when doing a clean.
    test_script_env.SideEffect("output\\main-tests.ilk", tests_obj)
    test_script_env.SideEffect("output\\main-tests.idb", tests_obj)
    test_script_env.SideEffect("output\\main-tests.pdb", tests_obj)
    test_script_env.SideEffect("output\\main-tests.mapS", tests_obj)
    test_script_env.Clean(tests_obj, "output\\main-tests.ilk")
    test_script_env.Clean(tests_obj, "output\\main-tests.idb")
    test_script_env.Clean(tests_obj, "output\\main-tests.pdb")
    test_script_env.Clean(tests_obj, "output\\main-tests.map")

def default_build_script(deps, output_name, env, part_name, install_alias, install_path, is_default, install_default, is_program = True):
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
    obj = env.Program(os.path.join('output', output_name), dependencies_out)
  else:
    obj = env.StaticLibrary(os.path.join('output', output_name), dependencies_out)

  iobj = None
  if install_default:
    iobj = env.Install(install_path, obj)
    Default(iobj)
  elif is_default:
    Default(obj)

  if install_alias is not None:
    env.Alias(install_alias, iobj or obj)

  return obj

def disassemble_cmd(target, source, env):
  disasm_cmd = 'ndisasm -b64 -a -p intel {obj_file} > {disassembly}'
  output_file = str(target[0]) + '.asm'
  os.system (disasm_cmd.format(obj_file = str(target[0]), disassembly = output_file))

  return None

def construct_variables(linux_build):
  var = Variables(["build_support/default_config.py", "variables.cache" ], ARGUMENTS)
  if not linux_build:
    var.AddVariables(
      BoolVariable("use_msvc",
                   "Use MSVC compiler. This will prevent compiling anything except the test scripts. Setting this to false is currently not supported.",
                   True))
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
else:
  print("Unknown build platform")
  exit(0)

config_env = construct_variables(linux_build)

main_build_script(linux_build, config_env)