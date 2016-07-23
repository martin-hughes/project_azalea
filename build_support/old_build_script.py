# Build script for Martin's Test Kernel

import os

GCC_CMD = "gcc -mno-red-zone -nostdlib -nostartfiles -nodefaultlibs -mcmodel=large -I . -o %(out)s -c %(in)s"
NASM_CMD = "nasm -f elf64 -o %(out)s %(in)s"
DISASM_CMD = "ndisasm -b64 %(in)s > %(out)s"

CMD_MAP = { "asm" : NASM_CMD,
            "cpp" : GCC_CMD }

FILE_TREE_TO_BUILD = [ "entry/entry.cpp", 
                       "entry/x64/entry-x64.asm",
                       "klib/panic/panic.cpp",
                       "klib/c_helpers/buffers.cpp",
                       "klib/lists/lists.cpp",
                       "klib/memory/memory.cpp",
                       "mem/x64/init.cpp",
                       "processor/x64/processor-x64.cpp",
                       "processor/x64/interrupts-x64.cpp",
                       "processor/x64/control-x64.asm",
                       "processor/x64/interrupts-low-x64.asm",
                       "processor/x64/gdt-x64.asm",
                        ]
OUTPUT_PREFIX = "output"
LINK_SCRIPT = "build_support/kernel_stage.ld"
LINK_CMD = "ld -M -T %(script)s -o %(output)s --start-group %(input)s --end-group"
LINKED_OUTPUT = "kernel64.sys"
ASM_OUTPUT = "kernel64.asm"
IMAGE_CMD = "build_support/old_copy_image.sh"

def main():
  # Expand this list into a complete set of files to build.
  FILES_TO_BUILD = FILE_TREE_TO_BUILD

  # Compilation pass. Store a list of built files so that we can link them all.
  all_output_files = [ ]
  for one_file in FILES_TO_BUILD:
    # Sort out the output filename, and add it to the list of things we're
    # building.
    output_file = os.path.join(OUTPUT_PREFIX, one_file)
    file_parts = os.path.splitext(output_file)
    file_base = file_parts[0]
    file_ext = file_parts[1]
    output_file = file_base + file_ext + ".o"
    all_output_files = all_output_files + [output_file]
    
    # Make sure the output path exists, otherwise GCC will complain.
    output_dir = os.path.dirname(output_file)
    if not os.path.exists(output_dir):
      os.makedirs(output_dir)
      
    # Decide what command to invoke.
    ext = os.path.splitext(one_file)[1][1:]
    build_command = CMD_MAP[ext]
    
    # Build the file.
    print os.path.basename(one_file)
    file_dict = {"in" : one_file, "out": output_file}
    build_command = build_command % file_dict
    os.system(build_command)

  # Link pass
  cmd_dict = {"script" : LINK_SCRIPT,
              "output" : LINKED_OUTPUT,
              "input" : " ".join(all_output_files)}
  link_command = LINK_CMD % cmd_dict
  print link_command
  print "Linking..."
  os.system(link_command)
  
  # Disassembly pass
  cmd_dict = {"in" : LINKED_OUTPUT,
              "out": ASM_OUTPUT}
  disasm_cmd = DISASM_CMD % cmd_dict
  print "Disassembling..."
  os.system(disasm_cmd)
  
  print "Preparing disk image"
  os.system(IMAGE_CMD)
  
  
if __name__ == "__main__":
  main()