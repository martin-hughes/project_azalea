def _setup_default_env(env):
  env['CC'] = 'gcc'  
  env['CXX'] = 'g++'
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
    env['CXXFLAGS'] = '-mno-red-zone -nostdlib -nostartfiles -nodefaultlibs -mcmodel=large -fno-exceptions -std=gnu++14 -U _LINUX -U __linux__ -D __AZALEA__'
    env['CFLAGS'] = '-mno-red-zone -nostdlib -nostartfiles -nodefaultlibs -mcmodel=large -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__'
    env['LINKFLAGS'] = "-T build_support/kernel_stage.ld --start-group"
    env['LINK'] = 'ld -Map output/kernel_map.map'
    
def setup_test_build_env(env):
    _setup_default_env(env)
    env['CXXFLAGS'] = '-std=gnu++14'
    env['LINKFLAGS'] = '-Wl,--start-group'