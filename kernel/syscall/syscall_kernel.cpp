// The kernel's system call interface, on the kernel side.

//#define ENABLE_TRACING

#include "user_interfaces/syscall.h"
#include "syscall/syscall_kernel.h"
#include "syscall/syscall_kernel-int.h"
#include "klib/klib.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/fs_file_interface.h"
#include "object_mgr/object_mgr.h"
#include "processor/x64/processor-x64.h"

#include <memory>

// The indicies of the pointers in this table MUST match the indicies given in syscall_user_low-x64.asm!
const void *syscall_pointers[] =
    { (void *)syscall_debug_output,

      // Handle management.
      (void *)syscall_open_handle,
      (void *)syscall_close_handle,
      (void *)syscall_read_handle,
      (void *)syscall_get_handle_data_len,
      (void *)syscall_write_handle,

      // Message passing.
      (void *)syscall_register_for_mp,
      (void *)syscall_send_message,
      (void *)syscall_receive_message_details,
      (void *)syscall_receive_message_body,
      (void *)syscall_message_complete,

      // Process & thread control
      (void *)syscall_create_process,
      (void *)syscall_start_process,
      (void *)syscall_stop_process,
      (void *)syscall_destroy_process,
      (void *)syscall_exit_process,

      (void *)syscall_create_thread,
      (void *)syscall_start_thread,
      (void *)syscall_stop_thread,
      (void *)syscall_destroy_thread,
      (void *)syscall_exit_thread,

      (void *)syscall_thread_set_tls_base,

      // Memory control,
      (void *)syscall_allocate_backing_memory,
      (void *)syscall_release_backing_memory,
      (void *)syscall_map_memory,
      (void *)syscall_unmap_memory,

      // Thread synchronization
      (void *)syscall_wait_for_object,
      (void *)syscall_futex_wait,
      (void *)syscall_futex_wake,

      // New syscalls:
      (void *)syscall_create_obj_and_handle,
      (void *)syscall_set_handle_data_len,
      (void *)syscall_set_startup_params,
      (void *)syscall_get_system_clock,
      (void *)syscall_rename_object,
      (void *)syscall_delete_object,
    };

const uint64_t syscall_max_idx = (sizeof(syscall_pointers) / sizeof(void *)) - 1;

bool syscall_is_um_address(const void *addr)
{
  uint64_t addr_l = reinterpret_cast<uint64_t>(addr);
  return !(addr_l & 0x8000000000000000ULL);
}

/// Write desired output to the system debug output
///
/// Transcribe directly from a user mode process into the kernel debug output. There might not always be a debug output
/// compiled in (although there always is in these early builds), in which case this system call will do nothing.
///
/// @param msg The message to be output - it is output verbatim, so an ASCII string is best here. Must be a pointer to
///            user space memory, to prevent any jokers outputting kernel secrets!
///
/// @param length The number of bytes to output. Maximum 1024.
///
/// @return ERR_CODE::INVALID_PARAM if either of the parameters isn't valid.
///         ERR_CODE::NO_ERROR otherwise (even if no output was actually made).
ERR_CODE syscall_debug_output(const char *msg, uint64_t length)
{
  KL_TRC_ENTRY;

  if (length > 1024)
  {
    return ERR_CODE::INVALID_PARAM;
  }
  if (msg == nullptr)
  {
    return ERR_CODE::INVALID_PARAM;
  }
  if (!SYSCALL_IS_UM_ADDRESS(msg))
  {
    // Don't output from kernel space!
    return ERR_CODE::INVALID_PARAM;
  }

  for (uint32_t i = 0; i < length; i++)
  {
    kl_trc_char(msg[i]);
  }

  KL_TRC_EXIT;

  return ERR_CODE::NO_ERROR;
}

/// @brief Open a handle corresponding to a System Tree object.
///
/// The handle can then be used by the calling process to reference the System Tree object without having to pass the
/// object itself back to the calling process.
///
/// @param[in] path The name of the System Tree object to generate a handle for.
///
/// @param[in] path_len The length of the string in path - the kernel may reject or be unable to handle very long
///                     strings and the call will fail.
///
/// @param[out] handle The handle for the calling process to use.
///
/// @return A suitable ERR_CODE value.
ERR_CODE syscall_open_handle(const char *path, uint64_t path_len, GEN_HANDLE *handle, uint32_t flags)
{
  ERR_CODE result;
  KL_TRC_ENTRY;
  task_thread *cur_thread = task_get_cur_thread();

  // Check parameters for robustness
  if ((path == nullptr) || (!SYSCALL_IS_UM_ADDRESS(path)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "path parameter invalid\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if ((handle == nullptr) || (!SYSCALL_IS_UM_ADDRESS(handle)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Handle parameter invalid\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (path_len == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Can't really handle zero-length paths\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  // All checks out, try to grab a system_tree object and allocate it a handle.
  else
  {
    std::shared_ptr<ISystemTreeLeaf> leaf;
    GEN_HANDLE new_handle;
    std::unique_ptr<char[]> buf = std::make_unique<char[]>(path_len + 1);
    kl_memcpy(path, buf.get(), path_len);
    buf[path_len] = 0;
    kl_string str_path(path);
    result = system_tree()->get_child(str_path, leaf);

    if (result == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Successfully got leaf object: ", leaf.get(), "\n");
      std::shared_ptr<IHandledObject> leaf_ptr = std::shared_ptr<IHandledObject>(leaf);
      new_handle = cur_thread->thread_handles.store_object(leaf_ptr);
      *handle = new_handle;

      KL_TRC_TRACE(TRC_LVL::EXTRA, "Correlated to handle ", new_handle, "\n");
    }
    else if ((result == ERR_CODE::NOT_FOUND) && ((flags & H_CREATE_IF_NEW) != 0))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Not found and asked to create\n");
      result = syscall_create_obj_and_handle(path, path_len, handle);
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to get leaf object, bail out\n");
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Close a previously opened handle.
///
/// After a handle is closed it can no longer be used by the calling process, and the value itself may be reused!
///
/// @param[in] handle The handle to close.
///
/// @return A suitable ERR_CODE value.
ERR_CODE syscall_close_handle(GEN_HANDLE handle)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::UNKNOWN;
  task_thread *cur_thread = task_get_cur_thread();
  std::shared_ptr<IHandledObject> obj;

  if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    obj = cur_thread->thread_handles.retrieve_object(handle);
    if (obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Object not found!\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found object: ", obj.get(), " - destroying\n");

      // Don't delete the object, let the reference counting mechanism take care of it as needed.
      cur_thread->thread_handles.remove_object(handle);
      obj = nullptr;

      result = ERR_CODE::NO_ERROR;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Read data from the associated object
///
/// Provided that "reading data" is meaningful to this System Tree object, read data and return it to the calling
/// process in the provided buffer.
///
/// The calling process is responsible for managing the memory allocated to the buffer, and data is copied in to the
/// buffer - modifying the buffer after this call does not cause corresponding changes in the System Tree object
/// itself.
///
/// @param[in] handle The handle of the object to read data from.
///
/// @param[in] start_offset How many bytes after the start of the file/pipe data/whatever to begin reading
///
/// @param[in] bytes_to_read The number of bytes to read from the object, if available.
///
/// @param[in] buffer The address of a buffer to write the read data in to.
///
/// @param[in] buffer_size The size of the buffer provided. The buffer must be sufficiently large to receive the data
///                        requested. Otherwise the read request is truncated to this length.
///
/// @param[out] bytes_read The number of bytes actually read in this request.
///
/// @return A suitable ERR_CODE value.
ERR_CODE syscall_read_handle(GEN_HANDLE handle,
                             uint64_t start_offset,
                             uint64_t bytes_to_read,
                             unsigned char *buffer,
                             uint64_t buffer_size,
                             uint64_t *bytes_read)
{
  KL_TRC_ENTRY;

  ERR_CODE result;
  task_thread *cur_thread = task_get_cur_thread();

  if ((buffer == nullptr) || !SYSCALL_IS_UM_ADDRESS(buffer))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "buffer is invalid\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if ((bytes_read == nullptr) || !SYSCALL_IS_UM_ADDRESS(bytes_read))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "bytes_read is invalid\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (buffer_size == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "bytes_to_read is invalid\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    // Parameters check out, try to read.
    std::shared_ptr<IHandledObject> leaf_ptr = cur_thread->thread_handles.retrieve_object(handle);
    std::shared_ptr<IReadable> file = std::dynamic_pointer_cast<IReadable>(leaf_ptr);
    KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieved leaf ", leaf_ptr.get(), " from OM\n");
    if (leaf_ptr == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Leaf object not found - bad handle\n");
      result = ERR_CODE::INVALID_PARAM;
    }
    else
    {
      if (file == nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Leaf is not a file, so can't be read.\n");
        result = ERR_CODE::INVALID_OP;
      }
      else
      {
        if (bytes_to_read > buffer_size)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Trimming bytes_to_read to max buffer length\n");
          bytes_to_read = buffer_size;
        }
        KL_TRC_TRACE(TRC_LVL::FLOW, "Going to attempt a read on file: ", file.get(), "\n");
        result = file->read_bytes(start_offset, bytes_to_read, buffer, buffer_size, *bytes_read);

        KL_TRC_TRACE(TRC_LVL::FLOW, "bytes read: ", *bytes_read, "\n");
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Return the number of bytes of data that could be read from a file, pipe or whatever other object.
///
/// The number of bytes that can be read depends on the handle type, but could be:
/// - The number of bytes in a file,
/// - The number of bytes waiting in a pipe,
/// - Any other indication for the maximum number of bytes available to syscall_read_handle().
///
/// @param[in] handle The handle to assess the size of.
///
/// @param[out] data_length The number of available bytes as described above.
///
/// @return A suitable ERR_CODE value.
ERR_CODE syscall_get_handle_data_len(GEN_HANDLE handle, uint64_t *data_length)
{
  KL_TRC_ENTRY;

  ERR_CODE result;
  task_thread *cur_thread = task_get_cur_thread();

  if ((data_length == nullptr) || !SYSCALL_IS_UM_ADDRESS(data_length))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "data_length ptr not valid.\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    std::shared_ptr<ISystemTreeLeaf> leaf =
      std::dynamic_pointer_cast<ISystemTreeLeaf>(cur_thread->thread_handles.retrieve_object(handle));
    KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieved leaf ", leaf.get(), " from OM\n");
    if (leaf == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Leaf object not found - bad handle\n");
      result = ERR_CODE::INVALID_PARAM;
    }
    else
    {
      std::shared_ptr<IBasicFile> file = std::dynamic_pointer_cast<IBasicFile>(leaf);
      if (file == nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Leaf is not a file, so can't tell size.\n");
        result = ERR_CODE::INVALID_OP;
      }
      else
      {
        file->get_file_size(*data_length);
        KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieved data length: ", *data_length, "\n");
        result = ERR_CODE::NO_ERROR;
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Write data to the associated object
///
/// Provided that "writing data" is meaningful to this System Tree object, write data from the provided buffer to the
/// object.
///
/// The calling process is responsible for managing the memory allocated to the buffer. The copy is complete before the
/// system call returns, so the calling process may free the buffer immediately.
///
/// @param[in] handle The handle of the object to write data to.
///
/// @param[in] start_offset How many bytes after the start of the file/pipe data/whatever to begin writing
///
/// @param[in] bytes_to_write The number of bytes to write to the object, if possible.
///
/// @param[in] buffer The address of a buffer to write the data from.
///
/// @param[in] buffer_size The size of the buffer provided. The buffer must be sufficiently large to contain the data
///                        requested. Otherwise the write request is truncated to this length.
///
/// @param[out] bytes_written The number of bytes actually written in this request.
///
/// @return A suitable ERR_CODE value.
ERR_CODE syscall_write_handle(GEN_HANDLE handle,
                              uint64_t start_offset,
                              uint64_t bytes_to_write,
                              unsigned char *buffer,
                              uint64_t buffer_size,
                              uint64_t *bytes_written)
{
  KL_TRC_ENTRY;

  ERR_CODE result;
  task_thread *cur_thread = task_get_cur_thread();

  if ((buffer == nullptr) || !SYSCALL_IS_UM_ADDRESS(buffer))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "buffer is invalid\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if ((bytes_written == nullptr) || !SYSCALL_IS_UM_ADDRESS(bytes_written))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "bytes_read is invalid\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (buffer_size == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "buffer_size is invalid\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    // Parameters check out, try to read.
    std::shared_ptr<ISystemTreeLeaf> leaf =
      std::dynamic_pointer_cast<ISystemTreeLeaf>(cur_thread->thread_handles.retrieve_object(handle));
    KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieved leaf ", leaf.get(), " from OM\n");
    if (leaf == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Leaf object not found - bad handle\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      std::shared_ptr<IWritable> file = std::dynamic_pointer_cast<IWritable>(leaf);
      if (file == nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Leaf is not writable.\n");
        result = ERR_CODE::INVALID_OP;
      }
      else
      {
        if (bytes_to_write > buffer_size)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Trimming bytes_to_write to max buffer length\n");
          bytes_to_write = buffer_size;
        }
        KL_TRC_TRACE(TRC_LVL::FLOW, "Going to attempt a write on file: ", file.get(), "\n");
        result = file->write_bytes(start_offset, bytes_to_write, buffer, buffer_size, *bytes_written);

        KL_TRC_TRACE(TRC_LVL::FLOW, "bytes written: ", *bytes_written, "\n");
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Configure the base address of TLS for this thread.
///
/// Threads generally define their thread-local storage relative to either FS or GS. It is difficult for them to set
/// the base address of those registers in user-mode, so this system call allows the kernel to do it on their behalf.
///
/// It should be noted that modern x64 processors allow user mode threads to do this via WRGSBASE (etc.) but QEMU
/// doesn't support these instructions, so we can't enable it in Azalea yet.
///
/// @param reg Which register to set.
///
/// @param value The value to load into the base of the required register.
///
/// @return ERR_CODE::INVALID_PARAM if either reg isn't valid, or value either isn't canonical or in user-space.
ERR_CODE syscall_thread_set_tls_base(TLS_REGISTERS reg, uint64_t value)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::NO_ERROR;

  if (!SYSCALL_IS_UM_ADDRESS(value) || !mem_is_valid_virt_addr(value))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid base address\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    switch(reg)
    {
    case TLS_REGISTERS::FS:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Setting FS base to ", value, "\n");
      proc_write_msr (PROC_X64_MSRS::IA32_FS_BASE, value);
      break;

    case TLS_REGISTERS::GS:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Writing GS base to ", value, "\n");
      proc_write_msr (PROC_X64_MSRS::IA32_GS_BASE, value);
      break;

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown register\n");
      result = ERR_CODE::INVALID_PARAM;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Create a new object in the system tree and retrieve a handle for it.
///
/// At present, only leaves will be created. The type of leaf will depend on the position in the tree the new object
/// is being created. For example, leaves created under a branch of the Mem FS will probably be Mem FS file leaves.
///
/// Leaves cannot be created at all places in the tree. For example, no new leaves can be added to a pipe branch.
///
/// @param path The position in the tree to create the new object.
///
/// @param path_len The length of the path string.
///
/// @param[out] handle The handle of the newly created object. If an object isn't created, this is left untouched.
///
/// @return A suitable error code.
ERR_CODE syscall_create_obj_and_handle(const char *path, uint64_t path_len, GEN_HANDLE *handle)
{
  ERR_CODE result = ERR_CODE::UNKNOWN;
  std::shared_ptr<ISystemTreeLeaf> new_leaf;
  std::shared_ptr<IHandledObject> new_leaf_ptr;
  kl_string req_path(path, path_len);
  GEN_HANDLE new_handle;
  task_thread *cur_thread = task_get_cur_thread();

  KL_TRC_ENTRY;

  if ((path == nullptr) ||
      !SYSCALL_IS_UM_ADDRESS(path) ||
      (path_len == 0) ||
      (handle == nullptr) ||
      !SYSCALL_IS_UM_ADDRESS(handle))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid parameters\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    result = system_tree()->create_child(req_path, new_leaf);
    if (result == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "New leaf created!\n");
      new_leaf_ptr = std::dynamic_pointer_cast<IHandledObject>(new_leaf);
      new_handle = cur_thread->thread_handles.store_object(new_leaf_ptr);
      *handle = new_handle;

      KL_TRC_TRACE(TRC_LVL::EXTRA, "Correlated to handle ", new_handle, "\n");
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Set the length of data associated with the given handle.
///
/// If the handle represents a file, then this is equivalent to setting the length of the file. Not all handle types
/// will support this operation, and the exact outcome depends on the handle type, file system type, etc.
///
/// @param handle The handle to set the data length of.
///
/// @param length The length of data area that should be associated with the handle.
///
/// @return A suitable error code.
ERR_CODE syscall_set_handle_data_len(GEN_HANDLE handle, uint64_t data_length)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::NO_ERROR;
  task_thread *cur_thread = task_get_cur_thread();
  std::shared_ptr<IHandledObject> obj;
  std::shared_ptr<IBasicFile> file;

  if (cur_thread != nullptr)
  {
    obj = cur_thread->thread_handles.retrieve_object(handle);
    if (obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No object for handle\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      file = std::dynamic_pointer_cast<IBasicFile>(obj);
      if (file == nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Object is not a file\n");
        result = ERR_CODE::INVALID_OP;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Attempting to set size\n");
        result = file->set_file_size(data_length);
      }
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No current thread defined - so no handles\n");
    result = ERR_CODE::INVALID_OP;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Rename an object within the System Tree
///
/// Currently, support for this is quite limited, and usually only permits objects to be renamed within their current
/// position in the tree. It might be better described as "syscall_move_object" in future.
///
/// @param old_name The current name of the object to rename.
///
/// @param new_name The name to rename the object to.
///
/// @return A suitable error code.
ERR_CODE syscall_rename_object(const char *old_name, const char *new_name)
{
  ERR_CODE result{ERR_CODE::UNKNOWN};

  KL_TRC_ENTRY;

  if (!SYSCALL_IS_UM_ADDRESS(old_name) ||
      !SYSCALL_IS_UM_ADDRESS(new_name) ||
      (old_name == nullptr) ||
      (new_name == nullptr))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Bad parameters\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt to rename\n");
    kl_string str_old(old_name);
    kl_string str_new(new_name);
    result = system_tree()->rename_child(old_name, new_name);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Remove an object from System Tree.
///
/// If there are any handles still open to the object, depending on the object and its parents, those handles may
/// remain valid until closed, or object deletion may fail until the handles are closed.
///
/// @param path The path to delete.
///
/// @return A suitable error code.
ERR_CODE syscall_delete_object(const char *path)
{
  ERR_CODE result{ERR_CODE::UNKNOWN};

  KL_TRC_ENTRY;

  if (!SYSCALL_IS_UM_ADDRESS(path) || (path == nullptr))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Bad path parameter\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Parameter OK, try to delete\n");
    kl_string str_path(path);
    result = system_tree()->delete_child(path);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
