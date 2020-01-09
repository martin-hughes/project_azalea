/// @file
/// @brief General functions for dealing with ELF objects in Project Azalea.
///
/// Much like all of the code in this folder, it will eventually be folded into System Tree.

//#define ENABLE_TRACING

#include <string>
#include <memory>
#include "klib/klib.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/fs_file_interface.h"
#include "system_tree/process/process.h"
#include "system_tree/process/process_elf_structs.h"

/// @brief Generic function pointer typedef.
typedef void (*fn_ptr)();

/// @brief Load an ELF binary file into a new process
///
/// Create a new process space, and load the binary file's contents in to it. The only ELF format files that can be
/// loaded successfully are those without any need for relocations or dynamic loading. Files with unsupported sections
/// may load but not correctly execute.
///
/// When this function returns, the process is ready to start, but is suspended.
///
/// @param binary_name The System Tree name for an ELF file to load into a new process.
///
/// @return A task_process control structure for the new process.
std::shared_ptr<task_process> proc_load_elf_file(std::string binary_name)
{
  KL_TRC_ENTRY;

  // Start by locating the file in System Tree.
  // Providing that it fits, allocate space for it.
  // Create a user and kernel mode mappings so the kernel can write to it.
  // Copy it into that mapping.
  // Release the kernel side of that mapping.

  std::shared_ptr<ISystemTreeLeaf> disk_prog;
  std::shared_ptr<IBasicFile> new_prog_file;
  uint64_t prog_size;
  uint64_t bytes_read;
  std::shared_ptr<task_process> new_proc;
  uint8_t* load_buffer;
  elf64_file_header *file_header;
  elf64_program_header *prog_header;
  uint64_t end_addr;
  uint64_t page_start_addr;
  void *backing_addr;
  void *kernel_write_window;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Attempting to load binary ", binary_name, "\n");

  ASSERT(system_tree()->get_child(binary_name, disk_prog) == ERR_CODE::NO_ERROR);

  // Check the file will fit into a single page. This means we know the copy below has enough space.
  // There's no technical reason why it must fit in one page, but it makes it easier for the time being.
  new_prog_file = std::dynamic_pointer_cast<IBasicFile>(disk_prog);
  ASSERT(new_prog_file != nullptr);
  new_prog_file->get_file_size(prog_size);
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Binary file size ", prog_size, "\n");
  ASSERT(prog_size < MEM_PAGE_SIZE);

  // Load the entire file into a buffer - it'll make it easier to process, but slower.
  load_buffer = new uint8_t[prog_size];
  ASSERT(new_prog_file->read_bytes(0, prog_size, load_buffer, prog_size, bytes_read) == ERR_CODE::NO_ERROR);
  ASSERT(bytes_read == prog_size);

  KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieved entire file\n");

  // Check that this is a valid ELF64 file.
  file_header = reinterpret_cast<elf64_file_header *>(load_buffer);
  ASSERT((file_header->ident[0] == 0x7f)
         && (file_header->ident[1] == 'E')
         && (file_header->ident[2] == 'L')
         && (file_header->ident[3] == 'F'));
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
  KL_TRC_TRACE(TRC_LVL::FLOW, "About to construct new process\n");
  new_proc = task_process::create(start_addr_ptr, false);
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Created new process with entry point ", start_addr_ptr, "\n");

  // The kernel does writes in its own address space, to avoid accidentally trampling over the current process.
  // Allocate an address to use for that.
  kernel_write_window = mem_allocate_virtual_range(1);

  // Cycle through the program headers, looking for segments to load.
  for (uint32_t i = 0; i < file_header->num_prog_hdrs; i++)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Looking at header idx ", i, "\n");
    prog_header = reinterpret_cast<elf64_program_header *>(file_header->prog_hdrs_off
                                                           + i * (file_header->prog_hdr_entry_size)
                                                           + reinterpret_cast<uint64_t>(load_buffer));
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Address in mem: ", prog_header, "\n");

    // At the moment, this is the only type that we'll load.
    if (prog_header->type == 1) //LOAD type
    {
      KL_TRC_TRACE(TRC_LVL::EXTRA, "---------------\n");
      KL_TRC_TRACE(TRC_LVL::FLOW, "Loading section\n");
      ASSERT(prog_header->req_phys_addr < 0x8000000000000000L);

      uint64_t copy_end_addr;
      uint64_t copy_length;
      uint64_t offset;
      uint64_t bytes_to_zero = 0;
      uint64_t bytes_written = 0;
      void *write_ptr;
      void *read_ptr;

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

      for (uint64_t this_page = page_start_addr; this_page < end_addr; this_page += MEM_PAGE_SIZE)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Writing on another page: ", this_page, "\n");

        // Is there already a page for this mapped into the process's address space? If not, create one. In all cases,
        // map it into the kernel's space so we can write onto it.
        backing_addr = mem_get_phys_addr(reinterpret_cast<void *>(this_page), new_proc.get());
        if (backing_addr == nullptr)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "No space for that allocated in the child process, grabbing a new page...\n");
          backing_addr = mem_allocate_physical_pages(1);

          KL_TRC_TRACE(TRC_LVL::EXTRA, "Allocating this page in the process's tables\n");
          mem_vmm_allocate_specific_range(this_page, 1, new_proc.get());

          KL_TRC_TRACE(TRC_LVL::EXTRA, "Mapping new page ", backing_addr, " to ", this_page, "\n");
          mem_map_range(backing_addr, reinterpret_cast<void *>(this_page), 1, new_proc.get());
        }

        KL_TRC_TRACE(TRC_LVL::EXTRA, "Mapping page ", backing_addr, " to ", kernel_write_window, " for kernel writing\n");
        mem_map_range(backing_addr, kernel_write_window, 1);

        // If there are still bytes need writing, then do it, otherwise skip to filling in zeros.
        if (bytes_written < prog_header->size_in_file)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Writing data\n");

          // In this copy, fill up until the end of the page.
          copy_length = MEM_PAGE_SIZE - offset;

          // However, if that is greater than the end of the required copying then only copy the actually required
          // number of bytes.
          if (this_page + offset + copy_length > copy_end_addr)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Adjusting length\n");
            copy_length = copy_end_addr - this_page - offset;
          }

          write_ptr = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(kernel_write_window) + offset);
          read_ptr = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(load_buffer)
                                              + prog_header->file_offset
                                              + bytes_written);
          KL_TRC_TRACE(TRC_LVL::EXTRA, "Copy data:\n----------\n");
          KL_TRC_TRACE(TRC_LVL::EXTRA, "Write pointer: ", write_ptr, "\n");
          KL_TRC_TRACE(TRC_LVL::EXTRA, "Read pointer: ", read_ptr, "\n");
          KL_TRC_TRACE(TRC_LVL::EXTRA, "Length: ", copy_length, "\n");
          kl_memcpy(read_ptr, write_ptr, copy_length);
          bytes_written += copy_length;
          offset += copy_length;

          KL_TRC_TRACE(TRC_LVL::EXTRA, "Copy complete\n");
        }

        // If all the code is written, fill in zeroes
        if ((bytes_written >= prog_header->size_in_file) && bytes_to_zero && (offset < MEM_PAGE_SIZE))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Writing zeroes\n");
          uint64_t bytes_now = MEM_PAGE_SIZE - offset;
          if (bytes_now > bytes_to_zero)
          {
            bytes_now = bytes_to_zero;
          }

          write_ptr = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(kernel_write_window) + offset);
          kl_memset(write_ptr, 0, bytes_now);
          bytes_to_zero -= bytes_now;
        }

        // Having done the writing, unmap it again.
        KL_TRC_TRACE(TRC_LVL::EXTRA, "Unmapping kernel side\n");
        mem_unmap_range(kernel_write_window, 1, nullptr, false);

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
