#ifndef PROCESSOR_INTERNAL_H
#define PROCESSOR_INTERNAL_H

// Declarations internal to the processor/task-manager library.

#include "processor.h"

// The execution context for a thread on x64. It should never be necessary to access any of these fields outside of the
// x64 part of the task manager.

struct task_x64_exec_context
{
    // These fields do what they say on the tin.
    void *stack_ptr;
    void *exec_ptr;
    void *cr3_value;
};

void *task_int_create_exec_context(ENTRY_PROC entry_point, task_thread *new_thread);

task_thread *task_get_next_thread();
extern "C" task_x64_exec_context *task_int_swap_task(unsigned long stack_ptr, unsigned long exec_ptr, unsigned long cr3_value);

void task_install_task_switcher();
void task_platform_init();

extern "C" void proc_handle_irq(unsigned char irq_number);
void proc_irq_slowpath_thread();

#endif
