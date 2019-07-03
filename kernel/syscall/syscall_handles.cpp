/// @file
/// @brief System call handlers that manipulate handles.

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
    kl_string str_path(path);
    result = system_tree()->get_child(str_path, leaf);

    if (result == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Successfully got leaf object: ", leaf.get(), "\n");
      std::shared_ptr<IHandledObject> leaf_ptr = std::shared_ptr<IHandledObject>(leaf);
      new_object.object_ptr = leaf_ptr;
      new_handle = cur_thread->thread_handles.store_object(new_object);
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
      KL_TRC_TRACE(TRC_LVL::FLOW, "Handle not provided, lookup object\n");
      result = system_tree()->get_child(kl_string(path), leaf);
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

#define CONV_TEST(obj, type) ((std::dynamic_pointer_cast<type>((obj))) != nullptr ? true : false)
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
