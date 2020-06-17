/// @file
/// @brief Declare the object_manager class

#pragma once

#include <map>
#include "handles.h"
#include "types/object_type.h"
#include "types/spinlock.h"

/// @brief Managers the relationship between handles an objects.
///
/// Each thread has it's own object manager, since handles are private to threads.
///
/// For more information, see [docs/components/object_mgr/Object Manager.md]
class object_manager
{
public:
  object_manager();
  ~object_manager();

  GEN_HANDLE store_object(object_data &object);
  void correlate_object(object_data &object, GEN_HANDLE handle);
  void decorrelate_object(GEN_HANDLE handle);
  std::shared_ptr<object_data> retrieve_object(GEN_HANDLE handle);
  std::shared_ptr<IHandledObject> retrieve_handled_object(GEN_HANDLE handle);
  void remove_object(GEN_HANDLE handle);

  void remove_all_objects();

private:
  std::map<GEN_HANDLE, std::shared_ptr<object_data>> object_store; ///< Stores pointers to all managed objects.

  ipc::raw_spinlock om_main_lock{0}; ///< Synchronising lock.

  std::shared_ptr<object_data> int_retrieve_object(GEN_HANDLE handle);
};
