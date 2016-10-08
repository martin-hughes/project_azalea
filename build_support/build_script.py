# Build script for the Project Azalea Kernel

import os

BUILD_CMD = "scons -Q -f build_support/scons_scripts/kernel"
BUILD_TEST_CMD = "scons -Q -f build_support/scons_scripts/tests"
DISASM_CMD = "ndisasm -b64 -a -p intel %(in)s > %(out)s"
LINKED_OUTPUT = "output/kernel64.sys"
ASM_OUTPUT = "output/kernel64.asm"
IMAGE_CMD = "bash build_support/copy_image.sh"

def main():
  # Build pass
  print "Building kernel..."
  os.system(BUILD_CMD)

  # Disassembly pass
  cmd_dict = {"in" : LINKED_OUTPUT,
              "out": ASM_OUTPUT}
  disasm_cmd = DISASM_CMD % cmd_dict
  print "Disassembling..."
  os.system(disasm_cmd)
  
  print "Preparing disk image"
  os.system(IMAGE_CMD)
  
  print "Building tests..."
  os.system(BUILD_TEST_CMD)
  
if __name__ == "__main__":
  main()
