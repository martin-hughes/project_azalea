// The kernel's system call interface, on the kernel side.

//#define ENABLE_TRACING

#include "syscall.h"
#include "syscall/syscall_kernel.h"
#include "syscall/syscall_kernel-int.h"
#include "klib/klib.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/fs_file_interface.h"
#include "object_mgr/object_mgr.h"

#include <memory>

// The indicies of the pointers in this table MUST match the indicies given in syscall_user_low-x64.asm!
const void *syscall_pointers[] =
    { (void *)syscall_debug_output,
      (void *)syscall_open_handle,
      (void *)syscall_close_handle,
      (void *)syscall_read_handle, };

const unsigned long syscall_max_idx = (sizeof(syscall_pointers) / sizeof(void *)) - 1;

bool syscall_is_um_address(const void *addr)
{
  unsigned long addr_l = reinterpret_cast<unsigned long>(addr);
  return !(addr_l & 0x8000000000000000ULL);
}
#define SYSCALL_IS_UM_ADDRESS(x) syscall_is_um_address(reinterpret_cast<const void *>((x)))

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
/// return ERR_CODE::INVALID_PARAM if either of the parameters isn't valid.
///        ERR_CODE::NO_ERROR otherwise (even if no output was actually made).
ERR_CODE syscall_debug_output(const char *msg, unsigned long length)
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

  for (int i = 0; i < length; i++)
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
/// return A suitable ERR_CODE value.
ERR_CODE syscall_open_handle(const char *path, unsigned long path_len, GEN_HANDLE *handle)
{
  ERR_CODE result;
  KL_TRC_ENTRY;

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
  // All checks out, try to grab a system_tree object and allocate it a handle.
  else
  {
    ISystemTreeLeaf *leaf;
    GEN_HANDLE new_handle;
    std::unique_ptr<char[]> buf = std::make_unique<char[]>(path_len + 1);
    kl_memcpy(path, buf.get(), path_len);
    buf[path_len + 1] = 0;
    kl_string str_path(path);
    result = system_tree()->get_leaf(str_path, &leaf);

    if (result == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Successfully got leaf object: ", leaf, "\n");

      new_handle = om_store_object(leaf);
      *handle = new_handle;

      KL_TRC_TRACE(TRC_LVL::EXTRA, "Correlated to handle ", new_handle, "\n");
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
/// return A suitable ERR_CODE value.
ERR_CODE syscall_close_handle(GEN_HANDLE handle)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::UNKNOWN;

  void *obj = om_retrieve_object(handle);
  if (obj == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Object not found!\n");
    result = ERR_CODE::NOT_FOUND;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found object: ", obj, " - destroying\n");
    om_remove_object(handle);

    // This conversion relies on the only objects being stored in the om being from SystemTree - which might not always
    // be true.
    ISystemTreeLeaf *obj_leaf = reinterpret_cast<ISystemTreeLeaf *>(obj);
    delete obj_leaf;
    obj_leaf = nullptr;

    result = ERR_CODE::NO_ERROR;
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
/// return A suitable ERR_CODE value.
ERR_CODE syscall_read_handle(GEN_HANDLE handle,
                             unsigned long start_offset,
                             unsigned long bytes_to_read,
                             unsigned char *buffer,
                             unsigned long buffer_size,
                             unsigned long *bytes_read)
{
  KL_TRC_ENTRY;

  ERR_CODE result;

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
  else
  {
    // Parameters check out, try to read.
    ISystemTreeLeaf *leaf = reinterpret_cast<ISystemTreeLeaf *>(om_retrieve_object(handle));
    KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieved leaf ", leaf, " from OM\n");
    if (leaf == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Leaf object not found - bad handle\n");
      result = ERR_CODE::INVALID_PARAM;
    }
    else
    {
      IBasicFile *file = dynamic_cast<IBasicFile *>(leaf);
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
        KL_TRC_TRACE(TRC_LVL::FLOW, "Going to attempt a read on file: ", file, "\n");
        result = file->read_bytes(0, bytes_to_read, buffer, buffer_size, *bytes_read);

        KL_TRC_TRACE(TRC_LVL::FLOW, "bytes read: ", *bytes_read, "\n");
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
