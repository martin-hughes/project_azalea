#include <azalea/azalea.h>
#include <memory.h>
#include <unistd.h>

// Known deficiencies:
// - Memory is not released from the calling process
// - Any kind of failure causes memory and / or process leaks.
// - What happens if sections overlap each other, particularly in memory?
// - This function is literally horrible. But it'll need a decent rewrite to support more featurefull ELF files, so a
//   rewrite can wait until then.

/// @brief Load an executable file from disk and execute it.
///
/// @param filename The name of the file to load.
///
/// @param name_length The number of characters in the filename.
///
/// @param proc_handle[out] Pointer to space to store the process handle that is returned.
///
/// @param argv Command line arguments, in the traditional C-style.
///
/// @param envp Environment variables, in the normal (but not standardised) C-style. If this pointer is nullptr, this
///             process's environment is copied to the child.
///
/// @return ERR_CODE::UNRECOGNISED if the requested file isn't a valid ELF file, or another suitable error code.
ERR_CODE exec_file(const char *filename,
                   uint16_t name_length,
                   GEN_HANDLE *proc_handle,
                   char * const argv[],
                   char * const envp[])
{
  GEN_HANDLE file_handle = 0;
  ERR_CODE result = ERR_CODE::NO_ERROR;
  elf64_file_header file_header;
  elf64_program_header prog_header;
  uint64_t copy_end_addr;
  uint64_t offset;
  uint64_t bytes_written = 0;
  void *page_ptr;
  uint64_t page_start_addr;
  uint64_t end_addr;
  uint32_t pages_reqd;
  uint64_t args_and_env_space;
  uint64_t cur_arg_num;
  uint64_t argc;
  uint64_t envc;
  char *write_ptr;
  char **args_and_env_idx;
  char *new_prog_argv_this_proc;
  uint64_t new_prog_argv;
  char *new_prog_environ_this_proc;
  uint64_t new_prog_environ;

  if ((filename == nullptr) || (proc_handle == nullptr))
  {
    return ERR_CODE::INVALID_PARAM;
  }

  result = syscall_open_handle(filename, name_length, &file_handle);
  if (result != ERR_CODE::NO_ERROR)
  {
    return result;
  }

  result = proc_read_elf_file_header(file_handle, &file_header);
  if (result != ERR_CODE::NO_ERROR)
  {
    syscall_close_handle(file_handle);
    return result;
  }

  result = syscall_create_process(reinterpret_cast<void *>(file_header.entry_addr), proc_handle);
  if (result != ERR_CODE::NO_ERROR)
  {
    syscall_close_handle(file_handle);
    return result;
  }

  for (uint32_t i = 0; i < file_header.num_prog_hdrs; i++)
  {
    result = proc_read_elf_prog_header(file_handle, &file_header, &prog_header, i);

    if (result != ERR_CODE::NO_ERROR)
    {
      break;
    }

    // At the moment, this is the only type that we'll load. Copy each section into memory.
    if (prog_header.type == 1) //LOAD type
    {
      if(prog_header.req_phys_addr >= 0x8000000000000000L)
      {
        result = ERR_CODE::UNRECOGNISED;
      }

      end_addr = prog_header.req_virt_addr + prog_header.size_in_mem;
      copy_end_addr = prog_header.req_virt_addr + prog_header.size_in_file;
      page_start_addr = prog_header.req_virt_addr - (prog_header.req_virt_addr % MEM_PAGE_SIZE);
      bytes_written = 0;
      pages_reqd = ((end_addr - page_start_addr) / MEM_PAGE_SIZE) + 1;
      offset = prog_header.req_virt_addr % MEM_PAGE_SIZE;

      page_ptr = nullptr;
      result = syscall_allocate_backing_memory(pages_reqd, &page_ptr);
      if (result != ERR_CODE::NO_ERROR)
      {
        break;
      }

      write_ptr = reinterpret_cast<char *>(page_ptr) + offset;

      memset(write_ptr, 0, prog_header.size_in_mem);
      result = syscall_read_handle(file_handle,
                                   prog_header.file_offset,
                                   prog_header.size_in_file,
                                   reinterpret_cast<unsigned char *>(write_ptr),
                                   (pages_reqd * MEM_PAGE_SIZE) - offset,
                                   &bytes_written);
      if (result != ERR_CODE::NO_ERROR)
      {
        break;
      }
      if (bytes_written != prog_header.size_in_file)
      {
        result = ERR_CODE::UNRECOGNISED;
      }

      result = syscall_map_memory(*proc_handle,
                                  reinterpret_cast<void *>(page_start_addr),
                                  pages_reqd * MEM_PAGE_SIZE,
                                  0,
                                  page_ptr);
      if (result != ERR_CODE::NO_ERROR)
      {
        break;
      }

      result = syscall_release_backing_memory(page_ptr);
      if (result != ERR_CODE::NO_ERROR)
      {
        if (result == ERR_CODE::NOT_FOUND)
        {
          result = ERR_CODE::INVALID_OP;
        }
        break;
      }
    }
  }

  // We don't *really* care if this fails, it just means a floating handle until this process exits.
  syscall_close_handle(file_handle);

  // Copy arguments and environment into the new process. First, calculate how much space we need. The first argument
  // is always the process name. We also need space for a char pointer, and the null pointer at the end of the pointer
  // list.
  //
  // Start with the process arguments.
  args_and_env_space = 0;
  args_and_env_space += (name_length + 1);
  args_and_env_space += (2 * sizeof(char *)); // One pointer for argv[0] and one for a null terminator for the list.

  cur_arg_num = 0;
  argc = 1;
  if (argv != nullptr)
  {
    while(argv[cur_arg_num] != nullptr)
    {
      args_and_env_space += (strlen(argv[cur_arg_num]) + 1);
      args_and_env_space += sizeof(char *);

      cur_arg_num++;
      argc++;
    };
  }

  // Now work on the environment.
  args_and_env_space += sizeof(char *); // Space for the null terminator.
  if (envp == nullptr)
  {
    envp = environ;
  }

  cur_arg_num = 0;
  envc = 0;
  if (envp != nullptr)
  {
    while(envp[cur_arg_num] != nullptr)
    {
      args_and_env_space += (strlen(envp[cur_arg_num]) + 1);
      args_and_env_space += sizeof(char *);
      envc++;
      cur_arg_num++;
    }
  }

  pages_reqd = ((args_and_env_space - 1) / MEM_PAGE_SIZE) + 1;
  page_ptr = nullptr;

  if (pages_reqd == 0)
  {
    // This shouldn't be possible!
    return ERR_CODE::UNKNOWN;
  }

  result = syscall_allocate_backing_memory(pages_reqd, &page_ptr);
  if (result != ERR_CODE::NO_ERROR)
  {
    // Exiting now leaves a partially created process. We should probably delete it.
    return result;
  }

  result = syscall_map_memory(*proc_handle,
                              reinterpret_cast<void *>(0x000000000F200000),
                              pages_reqd * MEM_PAGE_SIZE,
                              0,
                              page_ptr);
  if (result != ERR_CODE::NO_ERROR)
  {
    // As above.
    return result;
  }

  args_and_env_idx = reinterpret_cast<char **>(page_ptr);
  write_ptr = reinterpret_cast<char *>(page_ptr);
  write_ptr = write_ptr + ((argc + 1) * sizeof(char *));
  new_prog_argv_this_proc = reinterpret_cast<char *>(page_ptr);
  new_prog_argv = 0x000000000F200000;
  new_prog_environ_this_proc = write_ptr;
  new_prog_environ = new_prog_argv + (reinterpret_cast<uint64_t>(write_ptr) - reinterpret_cast<uint64_t>(page_ptr));
  write_ptr = write_ptr + ((envc + 1) * sizeof(char *));

  for (uint64_t i = 0; i < argc; i++)
  {
    args_and_env_idx[i] = reinterpret_cast<char *>(new_prog_argv +
                          (reinterpret_cast<uint64_t>(write_ptr) - reinterpret_cast<uint64_t>(page_ptr)));
    if (i == 0)
    {
      memcpy(write_ptr, filename, strlen(filename) + 1);
      write_ptr = write_ptr + (strlen(filename) + 1);
    }
    else
    {
      memcpy(write_ptr, argv[i - 1], strlen(argv[i - 1]) + 1);
      write_ptr = write_ptr + (strlen(argv[i - 1]) + 1);
    }
  }

  args_and_env_idx[argc] = nullptr;

  for (uint64_t i = 0; i < envc; i++)
  {
    args_and_env_idx[argc + 1 + i] = reinterpret_cast<char *>(new_prog_argv +
                                     (reinterpret_cast<uint64_t>(write_ptr) - reinterpret_cast<uint64_t>(page_ptr)));
    memcpy(write_ptr, environ[i], strlen(environ[i]) + 1);
    write_ptr += (strlen(environ[i]) + 1);
  }
  args_and_env_idx[argc + envc + 1] = nullptr;

  syscall_release_backing_memory(page_ptr);

  result = syscall_set_startup_params(*proc_handle, argc, new_prog_argv, new_prog_environ);
  if (result != ERR_CODE::NO_ERROR)
  {
    return result;
  }

  result = syscall_start_process(*proc_handle);
  return result;
}