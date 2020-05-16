/// @file
/// @brief x64-specific part of the task manager

//#define ENABLE_TRACING
//#define TASK_SWAP_SANITY_CHECKS

#include "klib/klib.h"

#include "processor/processor.h"
#include "processor/processor-int.h"
#include "processor/x64/processor-x64.h"
#include "processor/x64/processor-x64-int.h"
#include "processor/x64/proc_interrupt_handlers-x64.h"
#include "mem/x64/mem-x64-int.h"
#include "processor/x64/pic/pic.h"

namespace
{
  const uint64_t DEF_RFLAGS_KERNEL = (uint64_t)0x200202;
  const uint64_t DEF_CS_KERNEL = 0x08;
  const uint64_t DEF_SS_KERNEL = 0x10;

  const uint64_t DEF_RFLAGS_USER = (uint64_t)0x203202;
  const uint64_t DEF_CS_USER = 0x18;
  const uint64_t DEF_SS_USER = 0x20;

  // Setting one const equal to another of a different size seems to confuse the linker...!
  const uint32_t TM_INTERRUPT_NUM = 32; //(const uint32_t) PROC_IRQ_BASE;
  const uint32_t TM_INT_INTERRUPT_NUM = 48; ///< A copy of the task mananger interrupt without the IRQ acknowledgement.
}

/// @brief Create a new x64 execution context
///
/// Create an entire x64 execution context that will cause entry_point to be executed within new_thread. This must only
/// be called once for each thread object.
///
/// @param entry_point The point where the new thread will begin executing
///
/// @param new_thread The thread that is having an execution context created for it
///
/// @param param Optional parameter to pass to the newly created thread.
///
/// @param stack_ptr Optional stack pointer for the thread to use. If none provided, the kernel allocates a stack. It
///                  is the caller's responsibility to deallocate this stack.
///
/// @return A pointer to the execution context. This is opaque to non-x64 code.
void *task_int_create_exec_context(ENTRY_PROC entry_point, task_thread *new_thread, uint64_t param, void *stack_ptr)
{
  task_x64_exec_context *new_context;
  task_process *parent_process;
  mem_process_info *memmgr_data;
  process_x64_data *memmgr_x64_data;
  uint64_t stack_long{0};

  KL_TRC_ENTRY;

  ASSERT(new_thread != nullptr);
  parent_process = new_thread->parent_process.get();
  ASSERT(parent_process != nullptr);
  memmgr_data = parent_process->mem_info;
  ASSERT(memmgr_data != nullptr);
  memmgr_x64_data = (process_x64_data *)memmgr_data->arch_specific_data;
  ASSERT(memmgr_data != nullptr);

  // Fill in the easy parts of the context
  new_context = new task_x64_exec_context;
  //new_context->exec_ptr = (void *)entry_point;
  KL_TRC_TRACE(TRC_LVL::FLOW, "Creating exec context for thread ", new_thread, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Exec pointer: ", entry_point, "\n");
  new_context->cr3_value = (void *)memmgr_x64_data->pml4_phys_addr;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "CR3: ", new_context->cr3_value, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Parameter - RDI: ", param, "\n");

  memset(new_context->saved_stack.fx_state, 0, sizeof(new_context->saved_stack.fx_state));

  new_context->saved_stack.r15 = 0;
  new_context->saved_stack.r14 = 0;
  new_context->saved_stack.r13 = 0;
  new_context->saved_stack.r12 = 0;
  new_context->saved_stack.r11 = 0;
  new_context->saved_stack.r10 = 0;
  new_context->saved_stack.r9 = 0;
  new_context->saved_stack.r8 = 0;
  new_context->saved_stack.rbp = 0;
  new_context->saved_stack.rdi = param;
  new_context->saved_stack.rsi = 0;
  new_context->saved_stack.rdx = 0;
  new_context->saved_stack.rcx = 0;
  new_context->saved_stack.rbx = 0;
  new_context->saved_stack.rax = 0;
  new_context->saved_stack.proc_rip = reinterpret_cast<uint64_t>(entry_point);

  new_context->fs_base = 0;
  new_context->gs_base = 0;

  new_context->owner_thread = new_thread;
  new_context->syscall_stack = proc_allocate_stack(true);
  new_context->orig_syscall_stack = new_context->syscall_stack;

  if (parent_process->kernel_mode)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Creating kernel mode context\n");
    //new_context->saved_stack.flags = DEF_RFLAGS_KERNEL;
    new_context->saved_stack.proc_rflags = DEF_RFLAGS_KERNEL;
    new_context->saved_stack.proc_cs = DEF_CS_KERNEL;
    new_context->saved_stack.proc_ss = DEF_SS_KERNEL;

    if (stack_ptr == nullptr)
    {
      // The stack is allocated and made ready to use by proc_x64_allocate_stack(). The allocated stacks are 16-byte
      // aligned. We deliberately offset a further 8 bytes. This is to simulate a `call` instruction to `entry_point`.
      KL_TRC_TRACE(TRC_LVL::FLOW, "Allocate stack\n");
      stack_long = reinterpret_cast<uint64_t>(proc_allocate_stack(true));
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Use provided stack\n");
      stack_long = reinterpret_cast<uint64_t>(stack_ptr);
    }

    new_context->saved_stack.proc_rsp = stack_long - 8;

    KL_TRC_TRACE(TRC_LVL::EXTRA, "Stack pointer:", new_context->saved_stack.proc_rsp, "\n");
  }
  else
  {
    //new_context->saved_stack.flags = DEF_RFLAGS_USER;
    new_context->saved_stack.proc_rflags = DEF_RFLAGS_USER;
    new_context->saved_stack.proc_cs = DEF_CS_USER + 3;
    new_context->saved_stack.proc_ss = DEF_SS_USER + 3;

    if (stack_ptr == nullptr)
    {
      // The stack is allocated and made ready to use by proc_x64_allocate_stack(). The allocated stacks are 16-byte
      // aligned. We deliberately offset a further 8 bytes. This is to simulate a `call` instruction to `entry_point`.
      KL_TRC_TRACE(TRC_LVL::FLOW, "Allocate stack\n");
      stack_long = reinterpret_cast<uint64_t>(proc_allocate_stack(false, parent_process));
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Use provided stack\n");
      stack_long = reinterpret_cast<uint64_t>(stack_ptr);
    }

    new_context->saved_stack.proc_rsp = stack_long;

    if (new_context->saved_stack.proc_rsp == 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No space for a new stack\n");
      delete new_context;
      new_context = nullptr;
    }
    else
    {
      // Finally, apply the same offset by the same 8 bytes as described for the kernel mode stack.
      new_context->saved_stack.proc_rsp -= 8;

      KL_TRC_TRACE(TRC_LVL::EXTRA, "Stack pointer:", new_context->saved_stack.proc_rsp, "\n");
    }
  }

  KL_TRC_EXIT;
  return (void *)new_context;
}

/// @brief Destroy an x64 execution context for a thread that is terminating.
///
/// @param old_thread The thread being destroyed.
void task_int_delete_exec_context(task_thread *old_thread)
{
  KL_TRC_ENTRY;

  ASSERT(old_thread->permit_running == false);
  ASSERT(old_thread->thread_destroyed);

  task_x64_exec_context *old_context = reinterpret_cast<task_x64_exec_context *>(old_thread->execution_context);

  proc_deallocate_stack(old_context->orig_syscall_stack);

  if (old_thread->parent_process->kernel_mode)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Deallocated kernel stack\n");
    proc_deallocate_stack(reinterpret_cast<void *>(old_context->saved_stack.proc_rsp));
  }

  delete old_context;
  old_thread->execution_context = nullptr;

  KL_TRC_EXIT;
}

/// @brief Set the command line and environment for a newly created process.
///
/// Azalea puts argc, and the argv and environ pointers into the registers for the first three parameters of a normal C
/// function, using the Linux style parameter passing scheme used throughout.
///
/// This can only be carried out on a process that hasn't started yet. This function simply assumes that the first
/// thread it finds for a process is the one that will execute the startup code, and sets up the registers
/// appropriately in that thread, so care should be taken if setting up multiple threads in a process before starting
/// it.
///
/// @param process The process to set the parameters for
///
/// @param argc Has the same meaning as argc in a normal C program.
///
/// @param argv Has the same meaning as argv in a normal C program. Must be a user mode pointer in the process's
///             address space, although the kernel doesn't enforce this - the program will simply crash if this is
///             wrong. This function does not copy the arguments into that space, it is assumed the program loader does
///             this.
///
/// @param env Has the same meaning as argv in a normal C program. Must be a user mode pointer in the process's address
///            space, although the kernel doesn't enforce this - the program will simply crash if this is wrong. This
///            function does not copy the arguments into that space, it is assumed the program loader does this.
void task_set_start_params(task_process *process, uint64_t argc, char **argv, char **env)
{
  KL_TRC_ENTRY;

  task_thread *first_thread;
  task_x64_exec_context *context;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Set up process: ", process, " with:\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "argc: ", argc, "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "argv: ", reinterpret_cast<uint64_t>(argv), "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "env: ", reinterpret_cast<uint64_t>(env), "\n");

  ASSERT(process != nullptr);
  ASSERT(process->child_threads.head != nullptr);
  first_thread = process->child_threads.head->item.get();
  ASSERT(first_thread != nullptr);
  ASSERT(first_thread->permit_running == false);

  context = reinterpret_cast <task_x64_exec_context *>(first_thread->execution_context);
  ASSERT(context != nullptr);

  context->saved_stack.rdi = argc;
  context->saved_stack.rsi = reinterpret_cast<uint64_t>(argv);
  context->saved_stack.rdx = reinterpret_cast<uint64_t>(env);

  KL_TRC_EXIT;
}

/// @brief Main task switcher
///
/// task_int_swap_task() is called by the timer interrupt. It saves the execution context of the thread currently
/// executing, selects the next one and provides the new execution context to the caller.
///
/// The action of choosing the next thread to execute is not platform specific, it is provided by generic code in
/// #task_get_next_thread.
///
/// @param stack_addr The stack pointer that provides the execution context that has just finished executing
///
/// @param cr3_value The value of CR3 used by the suspended thread
///
/// @return The execution context for the caller to begin executing.
task_x64_exec_context *task_int_swap_task(uint64_t stack_addr, uint64_t cr3_value)
{
  task_thread *current_thread;
  task_x64_exec_context *current_context;
  task_x64_exec_context *next_context;
  task_thread *next_thread;
  void *stack_ptr = reinterpret_cast<void *>(stack_addr);

  KL_TRC_ENTRY;

  current_thread = task_get_cur_thread();
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Current: ", current_thread, " (", stack_ptr, ")\n");
  if (current_thread != nullptr)
  {
    current_context = reinterpret_cast<task_x64_exec_context *>(current_thread->execution_context);
    current_context->cr3_value = (void *)cr3_value;

    memcpy(&(current_context->saved_stack), stack_ptr, sizeof(task_x64_saved_stack));

    current_context->fs_base = proc_read_msr(PROC_X64_MSRS::IA32_FS_BASE);
    current_context->gs_base = proc_read_msr(PROC_X64_MSRS::IA32_GS_BASE);

#ifdef TASK_SWAP_SANITY_CHECKS
    ASSERT((reinterpret_cast<uint64_t>(current_context->cr3_value) & 0xFFFFFFFF00000000) == 0);
    ASSERT(current_context->saved_stack.proc_cs < 100);
    ASSERT(current_context->saved_stack.proc_ss < 100);
    if (current_thread->parent_process->kernel_mode)
    {
      ASSERT((current_context->fs_base == 0) || (current_context->fs_base > 0xFFFFFFFF00000000));
      ASSERT((current_context->gs_base == 0) || (current_context->gs_base > 0xFFFFFFFF00000000));
      ASSERT(current_context->saved_stack.proc_rip > 0xFFFFFFFF00000000);
      ASSERT(current_context->saved_stack.proc_rsp > 0xFFFFFFFF00000000);
    }
    else
    {
      ASSERT((current_context->fs_base == 0) || (current_context->fs_base < 0xFFFFFFFF00000000));
      ASSERT((current_context->gs_base == 0) || (current_context->gs_base < 0xFFFFFFFF00000000));
    }
#endif
  }
  else
  {
    // Include this, rather than putting tracing in the if branch above, to avoid tracing the same info on every
    // context switch.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Not storing old thread\n");
  }

  next_thread = task_get_next_thread();
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Next: ", next_thread, "\n");
  next_context = reinterpret_cast<task_x64_exec_context *>(next_thread->execution_context);

  // The task switch interrupt uses the interrupt stack table mechanism, so each time the interrupt is called we use
  // the same part of memory, which is always in the kernel context. However, we want to adjust the return address to
  // be that of the next scheduled task. We could switch the stack pointer to point at the saved stack structure, but
  // that requires aligning the structure appropriately. Instead, simply blat away the old stack with the stack
  // corresponding to the task we want to switch to.
  memcpy(stack_ptr, &(next_context->saved_stack), sizeof(task_x64_saved_stack));

  // Save the thread context's address in IA32_KERNEL_GS_BASE in order that the processor can uniquely identify the
  // thread without having to look in a list (which is subject to threads moving between processors whilst looking in
  // the list.
  proc_write_msr(PROC_X64_MSRS::IA32_KERNEL_GS_BASE, reinterpret_cast<uint64_t>(next_thread->execution_context));

  // We also need to make sure the base values of FS and GS are set as needed.
  proc_write_msr(PROC_X64_MSRS::IA32_FS_BASE, next_context->fs_base);
  proc_write_msr(PROC_X64_MSRS::IA32_GS_BASE, next_context->gs_base);

  // Only processor 0 directly receives timer interrupts. In order to trigger scheduling on all other processors, send
  // them an IPI for the correct vector.
  if (proc_mp_this_proc_id() == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Sending broadcast IPI\n");
    proc_send_ipi(0, PROC_IPI_SHORT_TARGET::ALL_EXCL_SELF, PROC_IPI_INTERRUPT::FIXED, TM_INTERRUPT_NUM, false);
  }

  KL_TRC_EXIT;

  return next_context;
}

/// @brief Install the task switching routine
///
/// Links the timer's interrupt with code that causes the task switching process to begin. Once this code executes, the
/// timer will fire and task switching occur, and this could happen at an arbitrary time.
///
/// We have not set up the kernel's entry code path to be one of the threads in the execution list, so at some point
/// the timer will fire and schedule another thread, and this code path will simply cease.
void task_install_task_switcher()
{
  KL_TRC_ENTRY;

  proc_configure_idt_entry(TM_INTERRUPT_NUM, 0, (void *)asm_task_switch_interrupt_irq, 3);
  proc_configure_idt_entry(TM_INT_INTERRUPT_NUM, 0, (void *)asm_task_switch_interrupt_noirq, 4);
  asm_proc_install_idt();

  KL_TRC_EXIT;
}

/// @brief Platform-specific initialisation needed for task switching to begin.
void task_platform_init()
{
  KL_TRC_ENTRY;

  proc_init_tss();

  KL_TRC_EXIT;
}

/// @brief Give up the rest of our time slice.
///
/// Signal the scheduler to run. It'll a new thread to run as usual, and it might choose this one to run again.
void task_yield()
{
  KL_TRC_ENTRY;

  // Signal ourselves with a task-switching interrupt and that'll allow the task manager to select a new thread to run
  // (which might be this one).
  static_assert(TM_INT_INTERRUPT_NUM == 0x30, "Check task manager interrupt");
  asm("int $0x30");

  KL_TRC_EXIT;
}

/// @brief Return pointer to the currently executing thread
///
/// @return The task_thread object of the executing thread on this processor, or NULL if we haven't got that far yet.
task_thread *task_get_cur_thread()
{
  KL_TRC_ENTRY;

  task_thread *ret_thread;
  task_x64_exec_context *context;

  context = reinterpret_cast<task_x64_exec_context *>(proc_read_msr(PROC_X64_MSRS::IA32_KERNEL_GS_BASE));
  if (context == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No running thread\n");
    ret_thread = nullptr;
  }
  else
  {
    ret_thread = context->owner_thread;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", ret_thread, "\n");
  KL_TRC_EXIT;

  return ret_thread;
}
