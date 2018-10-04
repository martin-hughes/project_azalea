import os
import platform

import build_support.dependencies as dependencies
import build_support.config as config

def main_build_script(linux_build):
  # Most components are excluded from the Windows build. Only the test program builds on Windows.
  if linux_build:
    # Kernel headers
    kernel_env = build_default_env(linux_build)
    headers = kernel_env.File(Glob("kernel/user_interfaces/*.h"))
    headers_folder = os.path.join(config.azalea_dev_folder, "include")
    ui_folder = os.path.join(headers_folder, "azalea")
    kernel_env.Install(ui_folder, headers)

    # Main kernel part
    kernel_env['CXXFLAGS'] = '-Wall -mno-red-zone -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -std=gnu++14 -U _LINUX -U __linux__ -D __AZALEA__ -D KL_TRACE_BY_SERIAL_PORT'
    kernel_env['CFLAGS'] = '-Wall -mno-red-zone -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__ -D KL_TRACE_BY_SERIAL_PORT'
    kernel_env['LINKFLAGS'] = "-T build_support/kernel_stage.ld --start-group"
    kernel_env['LINK'] = 'ld -Map output/kernel_map.map'
    kernel_env.AppendENVPath('CPATH', '#/kernel')
    kernel_obj = default_build_script(dependencies.kernel, "kernel64.sys", kernel_env, "kernel")
    kernel_install_obj = kernel_env.Install(config.disk_image_folder, kernel_obj)
    kernel_env.AddPostAction(kernel_obj, disasm_action)
    kernel_env.AddPostAction(kernel_install_obj, disk_build_action)

    # Init program
    init_prog_env = build_default_env(linux_build)
    init_prog_env['CXXFLAGS'] = '-Wall -mno-red-zone -nostdinc -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -std=gnu++14 -U _LINUX -U __linux__ -D __AZALEA__ -D KL_TRACE_BY_SERIAL_PORT'
    init_prog_env['CFLAGS'] = '-Wall -mno-red-zone -nostdinc -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__ -D KL_TRACE_BY_SERIAL_PORT'
    init_prog_env['LIBPATH'] = config.libc_location
    init_prog_env.AppendENVPath('CPATH', os.path.join(config.libc_location, "include"))
    init_prog_env.AppendENVPath('CPATH', headers_folder)
    init_prog_env['LINK'] = 'ld -Map output/init_program.map'
    init_prog_env['LIBS'] = [ 'azalea_libc', ]
    init_prog_obj = default_build_script(dependencies.init_program, "initprog", init_prog_env, "init_program")
    init_install_obj = init_prog_env.Install(config.disk_image_folder, init_prog_obj)
    init_prog_env.AddPostAction(init_prog_obj, disasm_action)
    init_prog_env.AddPostAction(init_install_obj, disk_build_action)

    kernel_env.Alias('install-headers', ui_folder)
    Default(kernel_install_obj)
    Default(init_install_obj)

  # Unit test program
  test_script_env = build_default_env(linux_build)

  additional_defines = ' -D AZALEA_TEST_CODE -D KL_TRACE_BY_STDOUT'
  if config.test_copy_mem_map_files:
    additional_defines += ' -D AZALEA_TEST_COPY_MEM_MAP_FILES'

  if linux_build:
    test_script_env['LINKFLAGS'] = '-L/usr/lib/llvm-3.8/lib/clang/3.8.0/lib/linux -Wl,--start-group'
    cxx_flags = '-g -O0 -std=gnu++14 -Wunknown-pragmas'
    if config.test_attempt_mem_leak_check:
      test_script_env['LIBS'].append ('clang_rt.asan-x86_64')
      cxx_flags = cxx_flags + ' -fsanitize=address'
    cxx_flags = cxx_flags + additional_defines
    exe_name = 'main-tests'
    test_script_env['LIBS'] = [ 'boost_iostreams', 'pthread' ]
    additional_include_tag = 'CPATH'
  else:
    additional_defines += ' -D _DEBUG /MTd'
    if config.test_attempt_mem_leak_check:
      additional_defines += ' -D UT_MEM_LEAK_CHECK'
    test_script_env['LINKFLAGS'] = '/DEBUG:FULL /MAP:output\\main-tests.map /INCREMENTAL /NOLOGO'
    cxx_flags = additional_defines + ' /nologo /EHac /Od /ZI /Fdoutput\\main-tests.pdb'
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

def default_build_script(deps, output_name, env, part_name):
  # Transform the name of dependencies so that the intermediate files end up in output/(part name)/... and remove the
  # SConscript part of the path.
  dependencies_out = [ ]
  for dep in deps:
    out_dir = os.path.join('output', part_name)
    out_dir = os.path.join(out_dir, dep.strip('#'))
    out_dir = os.path.dirname(out_dir)
    dep_out = env.SConscript(dep, 'env', variant_dir=out_dir, duplicate=0)
    dependencies_out.append(dep_out)

  return env.Program(os.path.join('output', output_name), dependencies_out)

def disk_image_builder(target, source, env):
  os.system('sync')
  os.system('cp kernel_disc_image.vdi kernel_disc_image_nonlive.vdi')
  return None

def disassemble_cmd(target, source, env):
  disasm_cmd = 'ndisasm -b64 -a -p intel {obj_file} > {disassembly}'
  output_file = str(target[0]) + '.asm'
  os.system (disasm_cmd.format(obj_file = str(target[0]), disassembly = output_file))

  return None

# Create a custom disassembly action that is quiet.
disasm_action = Action(disassemble_cmd, "Disassembling $TARGET")

# Likewise for the disk image disk_image_builder
disk_build_action = Action(disk_image_builder, "Creating disk image")

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

main_build_script(linux_build)