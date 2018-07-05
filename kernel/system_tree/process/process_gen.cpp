/// @file
/// @brief General functions for dealing with processes in Project Azalea
///
/// These functions are intended for things like:
/// - Loading the processes
/// - Interacting with their memory space
/// - The high level interface for starting, stopping or otherwise communicating with them.
///
/// Lower level functionality remains with the code in /processor.
///
/// This distinction isn't particularly clear at the moment, and the aim for the future is to make this code part of
/// System Tree (hence its location)

#define ENABLE_TRACING

#include "klib/klib.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/fs_file_interface.h"
#include "system_tree/process/process.h"

typedef void (*fn_ptr)();

/// @brief Load a flat binary file into a new process
///
/// Create a new process space, and load the binary file's contents in to it. Flat binary files do not have any
/// additional information associated with them, so they are always loaded at an address of 0x200000.
///
/// When this function returns, the process is ready to start, but is suspended.
///
/// param binary_name The System Tree name for a binary file to load into a new process.
///
/// return A task_process control structure for the new process.
task_process *proc_load_binary_file(kl_string binary_name)
{
  KL_TRC_ENTRY;

  // Start by locating the file in System Tree.
  // Providing that it fits, allocate space for it.
  // Create a user and kernel mode mappings so the kernel can write to it.
  // Copy it into that mapping.
  // Release the kernel side of that mapping.

  INCOMPLETE_CODE("Doesn't deal with allocating virtual address within the process");

  ISystemTreeLeaf *disk_prog;
  IBasicFile *new_prog_file;
  uint64_t prog_size;
  uint64_t bytes_read;
  task_process *new_proc;
  const uint64_t start_addr = 0x200000;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Attempting to load binary ", binary_name, "\n");

  ASSERT(system_tree()->get_leaf(binary_name, &disk_prog) == ERR_CODE::NO_ERROR);

  // Check the file will fit into a single page. This means we know the copy below has enough space.
  // There's no technical reason why it must fit in one page, but it makes it easier for the time being.
  new_prog_file = dynamic_cast<IBasicFile*>(disk_prog);
  ASSERT(new_prog_file != nullptr);
  new_prog_file->get_file_size(prog_size);
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Binary file size ", prog_size, "\n");
  ASSERT(prog_size < MEM_PAGE_SIZE);

  // Create a new user mode process.
  fn_ptr start_addr_ptr = reinterpret_cast<fn_ptr>(start_addr);
  new_proc = task_create_new_process(start_addr_ptr, false);
  ASSERT(new_proc != nullptr);

  // Allocate it some memory for the code to go in, and allow the kernel to access it.
  void *physical_page = mem_allocate_physical_pages(1);
  void *kernel_virtual_page = mem_allocate_virtual_range(1);
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Physical page to use", (uint64_t)physical_page, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Kernel virtual page", (uint64_t)kernel_virtual_page, "\n");
  mem_map_range(physical_page, kernel_virtual_page, 1);
  KL_TRC_TRACE(TRC_LVL::FLOW, "First map complete\n");

  // Copy the program from disk into that space
  ASSERT(new_prog_file->read_bytes(0,
                                   prog_size,
                                   static_cast<uint8_t *>(kernel_virtual_page),
                                   prog_size,
                                   bytes_read)
         == ERR_CODE::NO_ERROR);
  ASSERT(bytes_read == prog_size);
  KL_TRC_TRACE(TRC_LVL::FLOW, "Program copied\n");

  // No need to access it from the kernel any more
  mem_unmap_range(kernel_virtual_page, 1);
  KL_TRC_TRACE(TRC_LVL::FLOW, "Kernel space unmapped\n");

  // In the context of the program, set up the virtual page allocation. The process starts at 2MB.
  mem_map_range(physical_page, (void *)0x200000, 1, new_proc);
  mem_deallocate_virtual_range(kernel_virtual_page,  1);
  KL_TRC_TRACE(TRC_LVL::FLOW, "User mode map complete.\n");

  KL_TRC_TRACE(TRC_LVL::EXTRA, "task_process ptr: ", new_proc, "\n");
  KL_TRC_EXIT;

  return new_proc;
}
