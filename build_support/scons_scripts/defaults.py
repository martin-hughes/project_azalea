def _setup_default_env(env):
  env['CC'] = 'clang'
  env['CXX'] = 'clang++'
  env['CPPPATH'] = '#'
  env['LINK'] = 'g++'
  env['AS'] = 'nasm'
  env['ASFLAGS'] = '-f elf64'
  env['RANLIBCOM'] = ''
  env['ASCOMSTR'] =   "Assembling:   $TARGET"
  env['CCCOMSTR'] =   "Compiling:    $TARGET"
  env['CXXCOMSTR'] =  "Compiling:    $TARGET"
  env['LINKCOMSTR'] = "Linking:      $TARGET"
  env['ARCOMSTR'] =   "(pre-linking) $TARGET"
  env['LIBS'] = [ ]
  
def setup_kernel_build_env(env):
    _setup_default_env(env)
    env['CXXFLAGS'] = '-mno-red-zone -nostdlib -mstackrealign -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -std=gnu++14 -U _LINUX -U __linux__ -D __AZALEA__'
    env['CFLAGS'] = '-mno-red-zone -nostdlib -mstackrealign -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__'
    env['LINKFLAGS'] = "-T build_support/kernel_stage.ld --start-group"
    env['LINK'] = 'ld -Map output/kernel_map.map'
    
def setup_test_build_env(env):
    _setup_default_env(env)
    env['CXXFLAGS'] = '-g -O0 -std=gnu++14 -fsanitize=address -fsanitize=leak -D AZALEA_TEST_CODE'
    env['LINKFLAGS'] = '-L/usr/lib/llvm-3.8/lib/clang/3.8.0/lib/linux -Wl,--start-group'
    env['LIBS'] = [ 'clang_rt.asan-x86_64' ]
    env.AppendENVPath('CPATH', "#/external/googletest/googletest/include")