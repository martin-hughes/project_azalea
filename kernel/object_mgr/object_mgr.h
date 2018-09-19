#ifndef __OBJECT_MGR_H
#define __OBJECT_MGR_H

#include "handles.h"
#include "ref_counter.h"
#include "object_type.h"
#include "klib/data_structures/red_black_tree.h"

class object_manager
{
public:
  object_manager();
  ~object_manager();

  GEN_HANDLE store_object(std::shared_ptr<IHandledObject> object_ptr);
  void correlate_object(std::shared_ptr<IHandledObject>, GEN_HANDLE handle);
  void decorrelate_object(GEN_HANDLE handle);
  std::shared_ptr<IHandledObject> retrieve_object(GEN_HANDLE handle);
  void remove_object(GEN_HANDLE handle);

  void remove_all_objects();

private:
  kl_rb_tree<GEN_HANDLE, std::shared_ptr<object_data>> object_store;

  kernel_spinlock om_main_lock;

  std::shared_ptr<object_data> int_retrieve_object(GEN_HANDLE handle);
};

#endif
