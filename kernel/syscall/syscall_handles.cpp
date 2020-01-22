/// @file
/// @brief System call handlers that manipulate handles.

//#define ENABLE_TRACING

#include <string>

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
/// @param flags Set to H_CREATE_IF_NEW to create a new file if this one doesn't exist.
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
    object_data new_object;
    std::shared_ptr<ISystemTreeLeaf> leaf;
    GEN_HANDLE new_handle;
    std::unique_ptr<char[]> buf = std::make_unique<char[]>(path_len + 1);
    kl_memcpy(path, buf.get(), path_len);
    buf[path_len] = 0;
    std::string str_path(path);

    KL_TRC_TRACE(TRC_LVL::FLOW, "Look for leaf with name: ", path, "\n");

    result = system_tree()->get_child(str_path, leaf);

    if (result == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Successfully got leaf object: ", leaf.get(), "\n");
      std::shared_ptr<IHandledObject> leaf_ptr = std::shared_ptr<IHandledObject>(leaf);
      new_object.object_ptr = leaf_ptr;
      new_handle = cur_thread->thread_handles.store_object(new_object);
      *handle = new_handle;

      KL_TRC_TRACE(TRC_LVL::EXTRA, "Correlated ", path, " to handle ", new_handle, "\n");
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
    obj = cur_thread->thread_handles.retrieve_handled_object(handle);
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
  std::string req_path(path, path_len);
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
    object_data new_object;

    result = system_tree()->create_child(req_path, new_leaf);
    if (result == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "New leaf created!\n");
      new_leaf_ptr = std::dynamic_pointer_cast<IHandledObject>(new_leaf);
      new_object.object_ptr = new_leaf_ptr;
      new_handle = cur_thread->thread_handles.store_object(new_object);
      *handle = new_handle;

      KL_TRC_TRACE(TRC_LVL::EXTRA, "Correlated to handle ", new_handle, "\n");
    }
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
/// @param old_name_len The number of bytes in old_name.
///
/// @param new_name The name to rename the object to.
///
/// @param new_name_len The number of bytes in new_name.
///
/// @return A suitable error code.
ERR_CODE syscall_rename_object(const char *old_name,
                               uint64_t old_name_len,
                               const char *new_name,
                               uint64_t new_name_len)
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
    std::string str_old(old_name, old_name_len);
    std::string str_new(new_name, new_name_len);
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
/// @param path_len How many bytes are in the path string?
///
/// @return A suitable error code.
ERR_CODE syscall_delete_object(const char *path, uint64_t path_len)
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
    std::string str_path{path, path_len};
    result = system_tree()->delete_child(path);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Retrieve basic properties about an object in System Tree.
///
/// @param handle The handle of an object to retrieve properties for. One of this or path must be set, but not both. If
///               a handle is provided and maps to a non-system tree object (which shouldn't happen but might do until
///               the kernel and arrangement of objects in System Tree is more developed) then the result is
///               ERR_CODE::NOT_FOUND - this function only deals with objects in System Tree.
///
/// @param path The path of an object in System Tree to retrieve properties for. Either this or handle must be set, but
///             not both.
///
/// @param path_length The number of bytes in the path string.
///
/// @param[out] props Basic properties about this object.
///
/// @return A suitable error code.
ERR_CODE syscall_get_object_properties(GEN_HANDLE handle,
                                       const char *path,
                                       uint64_t path_length,
                                       object_properties *props)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  bool path_is_valid{false};
  std::shared_ptr<ISystemTreeLeaf> leaf;
  std::shared_ptr<IHandledObject> obj;
  task_thread *cur_thread = task_get_cur_thread();

  KL_TRC_ENTRY;

  path_is_valid = SYSCALL_IS_UM_ADDRESS(path) && (path != nullptr) && (path_length > 0);

  if (((handle != 0) && (path_is_valid)) ||
      ((handle == 0) && (!path_is_valid)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Only one of handle or path must be set!\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (!SYSCALL_IS_UM_ADDRESS(props))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "props must be a user-mode address\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Checks OK\n");
    if (handle == 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Handle not provided, lookup object: ", path, "\n");
      result = system_tree()->get_child(std::string(path, path_length), leaf);
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieve object\n");
      obj = cur_thread->thread_handles.retrieve_handled_object(handle);
      leaf = std::dynamic_pointer_cast<ISystemTreeLeaf>(obj);
      if (leaf == nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid handle\n");
        result = ERR_CODE::INVALID_PARAM;
      }
    }

    switch (result)
    {
    case ERR_CODE::NO_ERROR:
      KL_TRC_TRACE(TRC_LVL::FLOW, "No error, determine properties\n");
      memset(props, 0, sizeof(object_properties));
      props->exists = true;

/// @cond
#define CONV_TEST(obj, type) ((std::dynamic_pointer_cast<type>((obj))) != nullptr ? true : false)
/// @endcond
      props->is_file = CONV_TEST(leaf, IBasicFile);
      props->is_leaf = !CONV_TEST(leaf, ISystemTreeBranch);
      props->readable = CONV_TEST(leaf, IReadable);
      props->writable = CONV_TEST(leaf, IWritable);
      break;

    case ERR_CODE::NOT_FOUND:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Object not found\n");
      memset(props, 0, sizeof(object_properties));
      props->exists = false;
      break;

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Some other error occurred\n");
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Enumerate names of children objects of a system tree branch.
///
/// Note that the results of this function are consistent at the point the function is called, but as other threads may
/// also be operating on the branch, it is not guaranteed that two consecutive calls to this function return the same
/// results.
///
/// @param handle Handle to a system tree branch to enumerate children for.
///
/// @param start_from Name of the first child to enumerate after. May be nullptr to start from the beginning.
///
/// @param start_from_len Number of bytes in start_from. If zero, or if start_from doesn't form a valid string, both
///                       are ignored and enumeration starts from the first child.
///
/// @param max_count The number of children to enumerate. If zero, all children are enumerated - provided that buffer
///                  provides enough space. If it doesn't, only as many as will fit in the buffer are enumerated.
///
/// @param buffer Pointer to storage space for enumerated child names. The buffer starts with an array of string
///               pointers pointing to the names of children. The last element in the list is a nullptr. The strings
///               containing names are stored after this array.
///
/// @param buffer_size Pointer to the size of the array given. After this function completes and if the return code is
///                    ERR_CODE::NO_ERROR, then this value is changed to equal the number of bytes required to fulfil
///                    the request - in this way, the caller can adjust the size of the array to suit the expected
///                    results.
///
/// @return A suitable error code. If ERR_CODE::NOT_SUPPORTED, then the handle does not point to a system tree branch.
ERR_CODE syscall_enum_children(GEN_HANDLE handle,
                               const char *start_from,
                               uint64_t start_from_len,
                               uint64_t max_count,
                               void *buffer,
                               uint64_t *buffer_size)
{
  ERR_CODE result{ERR_CODE::UNKNOWN};
  uint64_t stored_buffer_size;
  task_thread *cur_thread{task_get_cur_thread()};
  std::shared_ptr<object_data> obj;
  std::shared_ptr<ISystemTreeBranch> branch;
  std::string start_from_s;
  uint64_t required_size{0};

  KL_TRC_ENTRY;

  ASSERT(cur_thread);

  obj = cur_thread->thread_handles.retrieve_object(handle);

  if ((!SYSCALL_IS_UM_ADDRESS(start_from) && (start_from != nullptr)) ||
      (!SYSCALL_IS_UM_ADDRESS(buffer) && (buffer != nullptr)) ||
      !SYSCALL_IS_UM_ADDRESS(buffer_size) ||
      (buffer_size == nullptr))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid pointers\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (!obj)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Handle not found\n");
    result = ERR_CODE::NOT_FOUND;
  }
  else if (!obj->object_ptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Handle not storing object\n");
    result = ERR_CODE::NOT_FOUND;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Look for branch\n");
    branch = std::dynamic_pointer_cast<ISystemTreeBranch>(obj->object_ptr);

    if (branch)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found branch - attempt enum\n");
      if ((start_from != nullptr) && (start_from_len > 0))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Populate start_from_s\n");
        start_from_s = std::string(start_from, start_from_len);
      }

      // Keep a copy of this to stop it potentially being changed by the child process during this function call.
      stored_buffer_size = *buffer_size;

      std::pair<ERR_CODE, std::vector<std::string>> res = branch->enum_children(start_from_s, max_count);

      result = res.first;

      if (result == ERR_CODE::NO_ERROR)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Successful enum, handle results\n");
        uint64_t num_to_store{0};
        // If we store any results, we need a null terminator for the array of string pointers, so consider that.
        uint64_t buffer_used{sizeof(char *)};
        char ** ptr_table{reinterpret_cast<char **>(buffer)};
        bool could_store_more{true};
        char *string_copy_ptr;

        required_size = buffer_used;

        // Make sure there's always at least a null terminator, if the buffer can take it!
        if ((buffer != nullptr) && (stored_buffer_size >= 8))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Store initial nullptr");
          ptr_table[0] = nullptr;
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't even store that...\n");
          could_store_more = false;
        }

        // Count how many bytes we'd need to store all the returned entries. At the same time, work out how many
        // entries would actually fit in to buffer.
        for (std::string n : res.second)
        {
          uint64_t bytes_this_name;

          KL_TRC_TRACE(TRC_LVL::FLOW, "Examine child: ", n, "\n");
          bytes_this_name = n.length() + sizeof(char *) + 1;

          required_size += bytes_this_name;

          if (could_store_more && ((buffer_used + bytes_this_name) <= stored_buffer_size))
          {
            // We keep track of the fact that we could store this path, but don't actually do it yet, because we don't
            // know how many entries will be in the table-of-pointers that will reside at the beginning of 'buffer'.
            KL_TRC_TRACE(TRC_LVL::FLOW, "Could store this name\n");
            buffer_used += bytes_this_name;
            num_to_store++;
          }
          else
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Out of buffer space\n");
            could_store_more = false;
          }
        }

        // Having worked out how many bytes we could store, do the copying.
        string_copy_ptr = reinterpret_cast<char *>(ptr_table + num_to_store + 1);
        for (uint64_t i = 0; i < num_to_store; i++)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Store name: ", res.second[i], "\n");
          std::string &this_name = res.second[i];
          ptr_table[i] = string_copy_ptr;
          ptr_table[i + 1] = nullptr;

          this_name.copy(ptr_table[i], std::string::npos);
          string_copy_ptr += this_name.length();
          *string_copy_ptr = 0;
          string_copy_ptr++;
        }

        // Now, tell the caller how many bytes would have been needed to get the whole table:
        *buffer_size = required_size;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Enum failed!\n");
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Attempting to enumerate non branch - invalid\n");
      result = ERR_CODE::INVALID_OP;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
