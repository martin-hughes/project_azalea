import os
import platform

import build_support.dependencies as dependencies
import build_support.config as config

def main_build_script(linux_build):
  # Most components and options are excluded from the Windows build. Only the test program builds on Windows.
  if linux_build:
    # API headers
    kernel_env = build_default_env(linux_build)
    headers = kernel_env.File(Glob("kernel/user_interfaces/*.h"))
    user_headers = kernel_env.File(Glob("user/libs/libazalea/azalea/*.h"))
    headers_folder = os.path.join(config.azalea_dev_folder, "include")
    ui_folder = os.path.join(headers_folder, "azalea")
    kernel_env.Install(ui_folder, headers)
    kernel_env.Install(ui_folder, user_headers)

    # Main kernel part
    kernel_env['CXXFLAGS'] = '-Wall -mno-red-zone -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -std=c++17 -U _LINUX -U __linux__ -D __AZALEA__ -D KL_TRACE_BY_SERIAL_PORT'
    kernel_env['CFLAGS'] = '-Wall -mno-red-zone -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__ -D KL_TRACE_BY_SERIAL_PORT'
    kernel_env['LINKFLAGS'] = "-T build_support/kernel_stage.ld --start-group"
    kernel_env['LINK'] = 'ld -Map output/kernel_map.map'
    kernel_env.AppendENVPath('CPATH', '#/kernel')
    kernel_obj = default_build_script(dependencies.kernel, "kernel64.sys", kernel_env, "kernel")
    kernel_install_obj = kernel_env.Install(config.system_root_folder, kernel_obj)
    kernel_env.AddPostAction(kernel_obj, disasm_action)

    # User mode API and programs environment
    user_mode_env = build_default_env(linux_build)
    user_mode_env['CXXFLAGS'] = '-Wall -mno-red-zone -nostdinc -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -std=c++17 -U _LINUX -U __linux__ -D __AZALEA__ -D KL_TRACE_BY_SERIAL_PORT'
    user_mode_env['CFLAGS'] = '-Wall -mno-red-zone -nostdinc -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__ -D KL_TRACE_BY_SERIAL_PORT'
    user_mode_env['LIBPATH'] = [config.libc_location, ]
    user_mode_env.AppendENVPath('CPATH', os.path.join(config.libc_location, "include"))
    user_mode_env.AppendENVPath('CPATH', headers_folder)
    user_mode_env['LINK'] = 'ld -Map output/init_program.map'

    # User mode part of the API
    user_api_obj = default_build_script(dependencies.user_mode_api, "azalea", user_mode_env, "api_library", False)

    user_mode_env['LIBS'] = [ 'azalea_libc', user_api_obj]

    # Init program
    init_deps = dependencies.init_program
    init_prog_obj = default_build_script(init_deps, "initprog", user_mode_env, "init_program")
    init_install_obj = user_mode_env.Install(config.system_root_folder, init_prog_obj)
    user_mode_env.AddPostAction(init_prog_obj, disasm_action)

    # Simple shell program
    shell_deps = dependencies.shell_program
    shell_prog_obj = default_build_script(shell_deps, "shell", user_mode_env, "simple_shell")
    shell_install_obj = user_mode_env.Install(config.system_root_folder, shell_prog_obj)

    echo_deps = dependencies.echo_program
    echo_prog_obj = default_build_script(echo_deps, "echo", user_mode_env, "echo_prog")
    echo_install_obj = user_mode_env.Install(config.system_root_folder, echo_prog_obj)

    # Install and other simple targets
    kernel_env.Alias('install-headers', ui_folder)
    PhonyTargets(kernel_env, start_demo = demo_machine_action)
    PhonyTargets(kernel_env, build_image = disk_build_action)
    Default(kernel_install_obj)
    Default(init_install_obj)
    Default(shell_install_obj)
    Default(echo_install_obj)

  # Unit test program
  test_script_env = build_default_env(linux_build)

  additional_defines = ' -D AZALEA_TEST_CODE -D KL_TRACE_BY_STDOUT'

  if linux_build:
    test_script_env['LINKFLAGS'] = '-L/usr/lib/llvm-6.0/lib/clang/6.0.0/lib/linux -Wl,--start-group'
    cxx_flags = '-g -O0 -std=c++17 -Wunknown-pragmas'
    test_script_env['LIBS'] = [ ]
    if config.test_attempt_mem_leak_check:
      test_script_env['LIBS'].append ('clang_rt.asan-x86_64')
      cxx_flags = cxx_flags + ' -fsanitize=address'

    cxx_flags = cxx_flags + additional_defines
    exe_name = 'main-tests'
    test_script_env['LIBS'].append([ 'libvirtualdisk', 'pthread' ])
    additional_include_tag = 'CPATH'
  else:
    additional_defines += ' -D _DEBUG /MTd'
    if config.test_attempt_mem_leak_check:
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

def disk_image_builder(target, source, env):
  os.system('build_support/create_disk_image.sh')
  return None

def disassemble_cmd(target, source, env):
  disasm_cmd = 'ndisasm -b64 -a -p intel {obj_file} > {disassembly}'
  output_file = str(target[0]) + '.asm'
  os.system (disasm_cmd.format(obj_file = str(target[0]), disassembly = output_file))

  return None

def demo_machine_cmd(target, source, env):
  qemu_params = ["qemu-system-x86_64",
                 "-drive file=fat:rw:fat-type=16:output/system_root,format=raw",
                 "-no-reboot",
                 "-smp cpus=2",
                 "-cpu Haswell,+x2apic",
                 "-serial stdio", # Could change this to -debugcon stdio to put the monitor on stdio.
                 "-device nec-usb-xhci",
                 "-kernel output/system_root/kernel64.sys",

                 # The following arguments are simply a reminder of useful options for temporary use while debugging.
                 #"-D /tmp/qemu.log",
                 #"-d int,pcall",
                 #"--trace events=/tmp/trcevents",
                 #"-device usb-kbd",
                ]
  qemu_cmd = " ".join(qemu_params) + " " + config.demo_machine_extra_params
  os.system(qemu_cmd)

  return None

def PhonyTargets(env, **kw):
  for target, action in kw.items():
      env.AlwaysBuild(env.Alias(target, [], action))

# Create actions for our custom commands.
disasm_action = Action(disassemble_cmd, "Disassembling $TARGET")
disk_build_action = Action(disk_image_builder, "Creating disk image")
demo_machine_action = Action(demo_machine_cmd, "Starting demo machine")

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