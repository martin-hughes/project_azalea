#include "processor/processor.h"

namespace
{
  uint64_t fake_ptr_target = 5;
  task_thread *fake_cur_thread = nullptr;
}

uint32_t proc_mp_proc_count()
{
  return 1;
}

uint32_t proc_mp_this_proc_id()
{
  return 0;
}

void task_platform_init()
{
  // Nothing to do.
}

void *task_int_create_exec_context(ENTRY_PROC entry_point, task_thread *new_thread)
{
  return (void *)&fake_ptr_target;
}

void task_int_delete_exec_context(task_thread *t)
{
  // Still nothing to do.
}

mem_process_info *mem_task_create_task_entry()
{
  return (mem_process_info *)&fake_ptr_target;
}

mem_process_info *mem_task_get_task0_entry()
{
  return (mem_process_info *)&fake_ptr_target;
}

void task_install_task_switcher()
{
  // Nothing to do.
}

task_thread *task_get_cur_thread()
{
  return fake_cur_thread;
}

void test_only_set_cur_thread(task_thread *thread)
{
  fake_cur_thread = thread;
}

void task_yield()
{
  // Not much that can be done here.
}