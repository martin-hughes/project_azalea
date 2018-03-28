# Build script for all of Project Azalea.

import os

config = {
  'build_command' : 'scons -Q -f {build_script}',
  'disasm_command' : 'ndisasm -b64 -a -p intel {output_file} > {disassembly}'
}

class gen_build_element:
  def __init__ (self, name):
    self.name = name

  def build(self):
    pass

class scons_build_element(gen_build_element):
  def __init__ (self, name, build_script, output_filename = None, disassembly = None):
    gen_build_element.__init__(self, name)

    self.name = name
    self.build_script = build_script
    self.output_filename = output_filename
    self.disassembly = disassembly

  def build(self):
    print ('Building: %s...' % self.name)
    build_cmd = config['build_command'].format(build_script = self.build_script)
    os.system(build_cmd)

    if (self.disassembly is not None) and (self.output_filename is not None):
      print ('Disassembling...')
      disasm_cmd = config['disasm_command'].format(output_file = self.output_filename, disassembly = self.disassembly)
      os.system(disasm_cmd)


class shell_script_build_element(gen_build_element):
  def __init__ (self, name, command):
    gen_build_element.__init__(self, name)

    self.command = command

  def build(self):
    print ('Building: %s...' % self.name)
    os.system(self.command)

kernel_element = scons_build_element('Main Kernel',
                                     'build_support/scons_scripts/kernel',
                                     'output/kernel64.sys',
                                     'output/kernel64.asm')

demo_prog_element = scons_build_element('Demo program',
                                        'build_support/scons_scripts/demo_program',
                                        'output/testprog',
                                        'output/testprog.asm')

unit_tests_element = scons_build_element('Unit tests', 'build_support/scons_scripts/tests')

image_build_element = shell_script_build_element('Disk image', 'bash build_support/copy_image.sh')

build_order = [
               kernel_element,
               demo_prog_element,
               unit_tests_element,
               image_build_element,
              ]

def main():
  for component in build_order:
    component.build()

if __name__ == "__main__":
  main()
