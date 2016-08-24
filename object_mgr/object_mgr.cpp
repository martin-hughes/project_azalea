/// @file
/// @brief Kernel's Object Manager
///
/// The Object Manager correlates handles and objects. Objects are any data object the user wishes to keep a reference
/// to. Users are responsible for ensuring that objects are removed from the Object Manager before destruction.
///
/// When an object is said to be "stored in OM" it does not mean that the object is in any way copied into OM. OM
/// simply stores a reference to the object (a pointer at the moment) which continues to live where it did before.
///
/// At the moment, this is implemented using the very slow method of a linked list. This is because I'm lazy.

#include "klib/klib.h"
#include "handles.h"
#include "object_mgr.h"
#include "object_type.h"

static klib_list om_main_list;
static kernel_spinlock om_main_lock;

object_data *om_int_retrieve_object(GEN_HANDLE handle);

/// @brief Initialise the object manager system
void om_gen_init()
{
  KL_TRC_ENTRY;

  klib_list_initialize(&om_main_list);
  klib_synch_spinlock_init(om_main_lock);

  KL_TRC_EXIT;
}

/// @brief Store an object in Object Manager
///
/// Stores an object in Object Manager and returns a new handle to reference it by.
///
/// @param object_ptr A pointer to the object to store in OM
///
/// @return A handle that correlates to object_ptr
GEN_HANDLE om_store_object(void *object_ptr)
{
  KL_TRC_ENTRY

  GEN_HANDLE new_handle = hm_get_handle();

  ASSERT(object_ptr != nullptr);

  om_correlate_object(object_ptr, new_handle);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "New handle: ", new_handle, "\n");
  KL_TRC_EXIT;

  return new_handle;
}

/// @brief Store an object in Object Manager with a known handle
///
/// In some cases, it is useful for the caller to have generated a handle for an object it wishes to store in OM. This
/// function stores the object and correlates it with the provided handle.
///
/// @param object_ptr A pointer to the object to be stored
///
/// @param handle The handle that should refer to object_ptr
void om_correlate_object(void *object_ptr, GEN_HANDLE handle)
{
  KL_TRC_ENTRY;

  object_data *new_object = new object_data;
  klib_list_item *new_item = new klib_list_item;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Object pointer: ", object_ptr, "\n");

  ASSERT(object_ptr != nullptr);

  new_object->object_ptr = object_ptr;
  new_object->handle = handle;
  new_object->owner_list_item = new_item;
  klib_list_item_initialize(new_item);
  new_item->item = new_object;

  klib_synch_spinlock_lock(om_main_lock);
  klib_list_add_tail(&om_main_list, new_item);
  klib_synch_spinlock_unlock(om_main_lock);

  KL_TRC_EXIT;
}

/// @brief Retrieve the object that correlates to handle
///
/// @param handle The handle to retrieve the corresponding object for
///
/// @return A pointer to the object stored in OM.
void *om_retrieve_object(GEN_HANDLE handle)
{
  KL_TRC_ENTRY;

  object_data *found_object;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Looking for handle ", handle, "\n");

  klib_synch_spinlock_lock(om_main_lock);
  found_object = om_int_retrieve_object(handle);
  klib_synch_spinlock_unlock(om_main_lock);

  ASSERT(found_object != nullptr);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Found object", found_object->object_ptr, "\n");
  KL_TRC_EXIT;
  return found_object->object_ptr;
}

/// @brief Remove an object from OM and destroy the handle
///
/// Removes the correlation between a handle and object, and frees the handle for re-use. It is up to the caller to
/// manage the lifetime of the associated object.
///
/// @param handle The handle to destroy
void om_remove_object(GEN_HANDLE handle)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Remove and destroy handle ", handle, "\n");
  om_decorrelate_object(handle);
  hm_release_handle(handle);

  KL_TRC_EXIT;
}

/// @brief Remove the correlation between handle and object, but leave both intact
///
/// Removes the correlation between a handle and object, but does not deallcoate the handle It is up to the caller to
/// manage the lifetime of both the object and handle.
///
/// @param handle The handle for the object to remove.
void om_decorrelate_object(GEN_HANDLE handle)
{
  KL_TRC_ENTRY;

  object_data *found_object;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Removing object with handle ", handle, "\n");

  klib_synch_spinlock_lock(om_main_lock);
  found_object = om_int_retrieve_object(handle);
  klib_list_remove(found_object->owner_list_item);
  klib_synch_spinlock_unlock(om_main_lock);

  delete found_object->owner_list_item;
  delete found_object;

  KL_TRC_EXIT;
}

/// @brief Retrieve all object data from OM
///
/// This function is internal to OM. It retrieves the underlying data structure storing a given object in OM. This
/// function contains no locking - **appropriate serialisation MUST be used**, only one function can call this one at a
/// time
///
/// @param handle The handle to retrieve data for
///
/// @return The underlying object data in OM.
object_data *om_int_retrieve_object(GEN_HANDLE handle)
{
  KL_TRC_ENTRY;

  klib_list_item *search_item;
  object_data *found_object = nullptr;
  object_data *check_object;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Handle to retrieve: ", handle, "\n");

  search_item = om_main_list.head;
  while(search_item != nullptr)
  {
    check_object = static_cast<object_data *>(search_item->item);

    if (check_object->handle == handle)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found object\n");
      found_object = check_object;
      break;
    }

    search_item = search_item->next;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Found item: ", found_object, "\n");
  KL_TRC_EXIT;

  return found_object;
}
