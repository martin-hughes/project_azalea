#include "processor/processor.h"

unsigned long fake_ptr_target = 5;

unsigned int proc_mp_proc_count()
{
  return 1;
}

unsigned int proc_mp_this_proc_id()
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
