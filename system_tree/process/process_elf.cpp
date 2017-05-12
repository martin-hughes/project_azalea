/// @file
/// @brief General functions for dealing with ELF objects in Project Azalea.
///
/// Much like all of the code in this folder, it will eventually be folded into System Tree.

#define ENABLE_TRACING

#include "klib/klib.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/fs_file_interface.h"
#include "system_tree/process/process.h"
#include "system_tree/process/process_elf_structs.h"

typedef void (*fn_ptr)();

/// @brief Load an ELF binary file into a new process
///
/// Create a new process space, and load the binary file's contents in to it. The only ELF format files that can be
/// loaded successfully are those without any need for relocations or dynamic loading. Files with unsupported sections
/// may load but not correctly execute.
///
/// When this function returns, the process is ready to start, but is suspended.
///
/// param binary_name The System Tree name for an ELF file to load into a new process.
///
/// return A task_process control structure for the new process.
task_process *proc_load_elf_file(kl_string binary_name)
{
  KL_TRC_ENTRY;

  // Start by locating the file in System Tree.
  // Providing that it fits, allocate space for it.
  // Create a user and kernel mode mappings so the kernel can write to it.
  // Copy it into that mapping.
  // Release the kernel side of that mapping.

  ISystemTreeLeaf *disk_prog;
  IBasicFile *new_prog_file;
  unsigned long prog_size;
  unsigned long bytes_read;
  task_process *new_proc;
  unsigned char* load_buffer;
  elf64_file_header *file_header;
  elf64_program_header *prog_header;
  unsigned long end_addr;
  unsigned long page_start_addr;
  void *backing_addr;
  void *kernel_write_window;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Attempting to load binary ", binary_name, "\n");

  ASSERT(system_tree()->get_leaf(binary_name, &disk_prog) == ERR_CODE::NO_ERROR);

  // Check the file will fit into a single page. This means we know the copy below has enough space.
  // There's no technical reason why it must fit in one page, but it makes it easier for the time being.
  new_prog_file = reinterpret_cast<IBasicFile*>(disk_prog);
  new_prog_file->get_file_size(prog_size);
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Binary file size ", prog_size, "\n");
  ASSERT(prog_size < MEM_PAGE_SIZE);

  // Load the entire file into a buffer - it'll make it easier to process, but slower.
  load_buffer = new unsigned char[prog_size];
  ASSERT(new_prog_file->read_bytes(0, prog_size, load_buffer, prog_size, bytes_read) == ERR_CODE::NO_ERROR);

  // Check that this is a valid ELF64 file.
  file_header = reinterpret_cast<elf64_file_header *>(load_buffer);
  ASSERT((file_header->ident[0] == 0x7f) &&
         (file_header->ident[1] == 'E') &&
         (file_header->ident[2] == 'L') &&
         (file_header->ident[3] == 'F'));
  ASSERT(file_header->ident[4] == 2); // 64-bit ELF.
  ASSERT(file_header->ident[5] == 1); // Little-endian.
  ASSERT(file_header->ident[6] == 1); // ELF version 1.
  ASSERT(file_header->type == 2); // Executable.
  ASSERT(file_header->version == 1); // ELF version 1 (again!)
  ASSERT((file_header->prog_hdrs_off > 0) && (file_header->prog_hdrs_off < (prog_size - ELF64_PROG_HDR_SIZE)));
  ASSERT(file_header->num_prog_hdrs > 1);
  ASSERT(file_header->entry_addr < 0x8000000000000000LL);
  ASSERT(file_header->file_header_size >= ELF64_FILE_HDR_SIZE);
  ASSERT(file_header->prog_hdr_entry_size >= ELF64_PROG_HDR_SIZE);

  // Create a task context with the correct entry point - this is needed before we can map pages to copy the image in
  // to.
  fn_ptr start_addr_ptr = reinterpret_cast<fn_ptr>(file_header->entry_addr);
  new_proc = task_create_new_process(start_addr_ptr, false);

  // The kernel does writes in its own address space, to avoid accidentally trampling over the current process.
  // Allocate an address to use for that.
  kernel_write_window = mem_allocate_virtual_range(1);

  // Cycle through the program headers, looking for segments to load.
  for (unsigned int i = 0; i < file_header->num_prog_hdrs; i++)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Looking at header idx ", i, "\n");
    prog_header = reinterpret_cast<elf64_program_header *>(file_header->prog_hdrs_off
                                                           + i * (file_header->prog_hdr_entry_size)
                                                           + reinterpret_cast<unsigned long>(load_buffer));
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Address in mem: ", prog_header, "\n");

    // At the moment, this is the only type that we'll load.
    if (prog_header->type == 1) //LOAD type
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Loading section\n");
      ASSERT(prog_header->req_phys_addr < 0x8000000000000000L);

      unsigned long copy_end_addr;
      unsigned long copy_length;
      unsigned long offset;
      unsigned long bytes_to_zero = 0;
      unsigned long bytes_written = 0;
      void *write_ptr;

      end_addr = prog_header->req_virt_addr + prog_header->size_in_mem;
      copy_end_addr = prog_header->req_virt_addr + prog_header->size_in_file;
      page_start_addr = prog_header->req_virt_addr - (prog_header->req_virt_addr % MEM_PAGE_SIZE);

      KL_TRC_TRACE(TRC_LVL::EXTRA, "Requested start address: ", prog_header->req_virt_addr, "\n");
      KL_TRC_TRACE(TRC_LVL::EXTRA, "Requested mem size: ", prog_header->size_in_mem, "\n");
      KL_TRC_TRACE(TRC_LVL::EXTRA, "Size in file: ", prog_header->size_in_file, "\n");

      KL_TRC_TRACE(TRC_LVL::EXTRA, "Start of first page: ", page_start_addr, "\n");
      KL_TRC_TRACE(TRC_LVL::EXTRA, "End address: ", copy_end_addr, "\n");

      if (prog_header->size_in_mem > prog_header->size_in_file)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Some area needs zero-ing\n");
        bytes_to_zero = prog_header->size_in_mem - prog_header->size_in_file;
      }

      offset = prog_header->req_virt_addr % MEM_PAGE_SIZE;

      for(unsigned long this_page = page_start_addr; this_page < end_addr; this_page += MEM_PAGE_SIZE)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Writing on another page: ", this_page, "\n");

        // Is there already a page for this mapped into the process's address space? If not, create one. In all cases,
        // map it into the kernel's space so we can write onto it.
        backing_addr = mem_get_phys_addr(reinterpret_cast<void *>(this_page), new_proc);
        if (backing_addr == nullptr)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "No space for that allocated in the child process, grabbing a new page...\n");
          backing_addr = mem_allocate_physical_pages(1);

          KL_TRC_TRACE(TRC_LVL::EXTRA, "Mapping new page ", backing_addr, " to ", this_page, "\n");
          mem_map_range(backing_addr, reinterpret_cast<void *>(this_page), 1, new_proc);
        }

        KL_TRC_TRACE(TRC_LVL::EXTRA, "Mapping page ", backing_addr, " to ", kernel_write_window, " for kernel writing\n");
        mem_map_range(backing_addr, kernel_write_window, 1);

        // If there are still bytes need writing, then do it, otherwise skip to filling in zeros.
        if (bytes_written < prog_header->size_in_file)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Writing data\n");
          copy_length = MEM_PAGE_SIZE - offset;
          if (this_page + copy_length > copy_end_addr)
          {
            copy_length = copy_end_addr - this_page;
          }
          KL_TRC_TRACE(TRC_LVL::EXTRA, "Length to copy now: ", copy_length, "\n");

          write_ptr = reinterpret_cast<void *>(reinterpret_cast<unsigned long>(kernel_write_window) + offset);
          KL_TRC_TRACE(TRC_LVL::EXTRA, "Write pointer: ", write_ptr, "\n");
          kl_memcpy(load_buffer + prog_header->file_offset + bytes_written, write_ptr, copy_length);
          bytes_written += copy_length;
          offset += copy_length;

          KL_TRC_TRACE(TRC_LVL::EXTRA, "Copy complete\n");
        }

        // If all the code is written, fill in zeroes
        if ((bytes_written >= prog_header->size_in_file) && bytes_to_zero && (offset < MEM_PAGE_SIZE))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Writing zeroes\n");
          unsigned long bytes_now = MEM_PAGE_SIZE - offset;
          if (bytes_now > bytes_to_zero)
          {
            bytes_now = bytes_to_zero;
          }

          write_ptr = reinterpret_cast<void *>(reinterpret_cast<unsigned long>(kernel_write_window) + offset);
          kl_memset(write_ptr, 0, bytes_now);
          bytes_to_zero -= bytes_now;
        }

        // Having done the writing, unmap it again.
        KL_TRC_TRACE(TRC_LVL::EXTRA, "Unmapping kernel side\n");
        mem_unmap_range(kernel_write_window, 1);

        offset = 0;
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Releasing kernel write window space\n");
  mem_deallocate_virtual_range(kernel_write_window, 1);
  delete[] load_buffer;

  KL_TRC_EXIT;

  return new_proc;

}
