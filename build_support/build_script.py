# Build script for the Project Azalea Kernel
#
# It wouldn't be totally unreasonable to make this a shell script, but one day it might have to think for itself!

import os

BUILD_CMD = "scons -Q -f build_support/scons_scripts/kernel"
BUILD_DEMO_CMD = "scons -Q -f build_support/scons_scripts/demo_program"
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
  
  print "Building demo program"
  os.system(BUILD_DEMO_CMD)
  cmd_dict = {"in" : "output/testprog",
              "out" : "output/testprog.asm"}
  os.system (DISASM_CMD % cmd_dict)
  
  print "Preparing disk image"
  os.system(IMAGE_CMD)
  
  print "Building tests..."
  os.system(BUILD_TEST_CMD)
  
if __name__ == "__main__":
  main()
