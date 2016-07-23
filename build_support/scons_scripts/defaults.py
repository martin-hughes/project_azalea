def setup_default_env(env):
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