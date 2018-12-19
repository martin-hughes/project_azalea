#include "test/test_core/test.h"

#include "processor/processor.h"
#include "processor/processor-int.h"
#include "processor/x64/processor-x64.h"

const uint16_t PROC_NUM_INTERRUPTS = 256;
const uint16_t PROC_NUM_IRQS = 16;
const uint16_t PROC_IRQ_BASE = 32;
proc_interrupt_data proc_interrupt_data_table[PROC_NUM_INTERRUPTS];

void test_init_proc_interrupt_table()
{
  proc_config_interrupt_table();
}

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
  return new char[8];
}

void task_int_delete_exec_context(task_thread *t)
{
  ASSERT(!t->permit_running);
  ASSERT(t->thread_destroyed);
  delete[] ((char *)t->execution_context);
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

// This function is deliberately empty. It can be used by functions needing to provide an entry point in test code
// where it is known that entry point is never actually executed - for example, while creating process or thread
// objects.
void dummy_thread_fn()
{
  // Doesn't do anything.
}

void proc_write_msr(PROC_X64_MSRS msr, uint64_t value)
{
  panic("Can't write MSRs in test code");
}

void task_set_start_params(task_process * process, uint64_t argc, char **argv, char **env)
{
  // Doesn't mean anything in the test scripts.
}