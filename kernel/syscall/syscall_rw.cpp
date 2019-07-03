/// @file
/// @brief System calls that read from, or write to, a system tree object.

// Known defects:
// - Seek behaviour is not unit tested in any way.

#include "user_interfaces/syscall.h"
#include "syscall/syscall_kernel.h"
#include "syscall/syscall_kernel-int.h"
#include "klib/klib.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/fs_file_interface.h"
#include "object_mgr/object_mgr.h"
#include "processor/x64/processor-x64.h"

#include <memory>
#include <cstring>

/// @brief Read data from the associated object
///
/// Provided that "reading data" is meaningful to this System Tree object, read data and return it to the calling
/// process in the provided buffer.
///
/// The calling process is responsible for managing the memory allocated to the buffer, and data is copied in to the
/// buffer - modifying the buffer after this call does not cause corresponding changes in the System Tree object
/// itself.
///
/// After a successful read, the underlying object's data pointer is adjusted by the number of bytes read, so that
/// sequential reads read adjacent sections of data (like the C fread function).
///
/// @param[in] handle The handle of the object to read data from.
///
/// @param[in] start_offset How many bytes after the object's current data pointer position to begin reading.
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
    std::shared_ptr<object_data> object = cur_thread->thread_handles.retrieve_object(handle);
    if (object != nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Got an object...\n");

      std::shared_ptr<IReadable> file = std::dynamic_pointer_cast<IReadable>(object->object_ptr);
      KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieved leaf ", file.get(), " from OM\n");
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
        result = file->read_bytes(start_offset + object->data.seek_position,
                                  bytes_to_read,
                                  buffer,
                                  buffer_size,
                                  *bytes_read);

        // There's no need to do locking on this field because handles are per-thread.
        object->data.seek_position += *bytes_read;

        KL_TRC_TRACE(TRC_LVL::FLOW, "bytes read: ", *bytes_read, "\n");
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Bad handle - object not found\n");
      result = ERR_CODE::INVALID_PARAM;
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
/// After a successful write, the underlying object's data pointer is adjusted by the number of bytes read, so that
/// sequential writes write adjacent sections of data (like the C fwrite function).
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
    std::shared_ptr<object_data> obj = cur_thread->thread_handles.retrieve_object(handle);

    if (obj)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found an object\n");
      std::shared_ptr<IWritable> file = std::dynamic_pointer_cast<IWritable>(obj->object_ptr);
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
        result = file->write_bytes(start_offset + obj->data.seek_position,
                                    bytes_to_write,
                                    buffer,
                                    buffer_size,
                                    *bytes_written);

        // There's no need to do locking on this field because handles are per-thread.
        obj->data.seek_position += *bytes_written;

        KL_TRC_TRACE(TRC_LVL::FLOW, "bytes written: ", *bytes_written, "\n");
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Handle not found\n");
      result = ERR_CODE::INVALID_PARAM;
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
    std::shared_ptr<object_data> obj = cur_thread->thread_handles.retrieve_object(handle);
    KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieved object data ", obj.get(), " from OM\n");
    if (obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Leaf object not found - bad handle\n");
      result = ERR_CODE::INVALID_PARAM;
    }
    else
    {
      std::shared_ptr<IBasicFile> file = std::dynamic_pointer_cast<IBasicFile>(obj->object_ptr);
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
  std::shared_ptr<object_data> obj;
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
      file = std::dynamic_pointer_cast<IBasicFile>(obj->object_ptr);
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

/// @brief Seek the handle's data pointer.
///
/// Provided the handle supports it, moves a handle's data pointer.
///
/// @param handle The handle to operate on
///
/// @param offset - If dir == FROM_CUR, seeks the data pointer from the current position.
///               - If dir == FROM_START, offset must be greater than or equal to zero and the data pointer is moved to
///                 offset bytes from the start of the handle's data.
///               - If dir == FROM_END, offset must be greater than or equal to zero and the data pointer is moved to
///                 offset bytes before the end of the handle's data.
///
/// @param dir The direction to move the data pointer, as described above.
///
/// @param[out] new_offset The absolute value of the handle's data pointer after this call completes. This is only
///             valid after a successful seek. This pointer may be nullptr, in which case the value of new_offset is
///             discarded.
///
/// @return A suitable error code.
ERR_CODE syscall_seek_handle(GEN_HANDLE handle, int64_t offset, SEEK_OFFSET dir, uint64_t *new_offset)
{
  ERR_CODE result;
  task_thread *cur_thread = task_get_cur_thread();

  if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else if ((!SYSCALL_IS_UM_ADDRESS(new_offset)) && (new_offset != nullptr))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "new_offset pointer is not valid\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    std::shared_ptr<object_data> obj = cur_thread->thread_handles.retrieve_object(handle);
    KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieved object data ", obj.get(), " from OM\n");
    if (obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Leaf object not found - bad handle\n");
      result = ERR_CODE::INVALID_PARAM;
    }
    else
    {
      std::shared_ptr<IBasicFile> file = std::dynamic_pointer_cast<IBasicFile>(obj->object_ptr);
      if (file == nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Leaf is not a file, so can't tell size / seek.\n");
        result = ERR_CODE::INVALID_OP;
      }
      else
      {
        uint64_t size;
        uint64_t proposed_offset;
        result = file->get_file_size(size);

        if (result == ERR_CODE::NO_ERROR)
        {
          KL_TRC_TRACE("Successfully got size, continue\n");

          switch (dir)
          {
          case SEEK_OFFSET::FROM_START:
            KL_TRC_TRACE(TRC_LVL::FLOW, "Seek from start\n");
            proposed_offset = offset;
            break;

          case SEEK_OFFSET::FROM_END:
            KL_TRC_TRACE(TRC_LVL::FLOW, "Seek from end\n");
            proposed_offset = size - offset;
            break;

          case SEEK_OFFSET::FROM_CUR:
            KL_TRC_TRACE(TRC_LVL::FLOW, "Seek from current\n");
            proposed_offset = obj->data.seek_position + offset;
            break;

          default:
            KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid seek direction\n");
            result = ERR_CODE::INVALID_PARAM;
          }

          if (result == ERR_CODE::NO_ERROR)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Valid offset direction, check new pointer\n");
            if ((proposed_offset >= 0) && (proposed_offset <= size))
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "In range, valid seek\n");
              result = ERR_CODE::NO_ERROR;
              obj->data.seek_position = proposed_offset;
              new_offset != nullptr ? *new_offset = proposed_offset : 0;
            }
            else
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "Not in range\n");
              result = ERR_CODE::OUT_OF_RANGE;
            }
          }
        }
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}