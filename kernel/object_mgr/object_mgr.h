#ifndef __OBJECT_MGR_H
#define __OBJECT_MGR_H

#include "handles.h"
#include "object_type.h"
#include "klib/data_structures/red_black_tree.h"
#include "klib/synch/kernel_locks.h"

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
  kl_rb_tree<GEN_HANDLE, std::shared_ptr<object_data>> object_store;

  kernel_spinlock om_main_lock;

  std::shared_ptr<object_data> int_retrieve_object(GEN_HANDLE handle);
};

#endif
