/// @file
/// @brief Allows test scripts to define an Azalea system suitable for their test
///
/// Test systems can be constructed with a mixture of live and dummy components.

#pragma once

#include <type_traits>
#include <memory>

#include "system_tree.h"
#include "processor.h"
#include "work_queue.h"
#include "test_core/test.h"

#include "types/device_interface.h"

template<typename wq, bool init_system_tree, bool init_task_man> class test_system_factory
{
  // System tree is a prerequisite of the task manager
  static_assert((init_task_man && init_system_tree) || !init_system_tree);

public:
  test_system_factory()
  {
    if constexpr (init_system_tree)
    {
      system_tree_init();
    }

    if constexpr (init_task_man)
    {
      system_process = task_init();
      test_only_set_cur_thread(system_process->child_threads.head->item.get());
    }

    create_work_queue();
  };

  virtual ~test_system_factory()
  {
    if (created_work_queue)
    {
      work::test_only_terminate_queue();
    }

    if constexpr (init_task_man)
    {
      test_only_set_cur_thread(nullptr);
      test_only_reset_task_mgr();
    }

    if constexpr (init_system_tree)
    {
      test_only_reset_system_tree();
    }

    test_only_reset_name_counts();
  };

protected:
  bool created_work_queue{false};
  std::shared_ptr<task_process> system_process;

  template<class Q = wq>
    typename std::enable_if<std::is_same<Q, void>::value, void>::type create_work_queue() { };

  template<class Q = wq>
    typename std::enable_if<!std::is_same<Q, void>::value, void>::type create_work_queue()
  {
    work::init_queue<wq>();
    created_work_queue = true;
  };

};
