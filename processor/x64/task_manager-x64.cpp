/// @file
/// @brief x64-specific part of the task manager

//#define ENABLE_TRACING

#include "klib/klib.h"

#include "processor/processor.h"
#include "processor/processor-int.h"
#include "processor/x64/processor-x64-int.h"
#include "mem/x64/mem-x64-int.h"
#include "processor/x64/pic/pic.h"

const unsigned int req_stack_entries = 22;
const unsigned int STACK_EFLAGS_OFFSET = 15;
const unsigned int STACK_RIP_OFFSET = 16;
const unsigned int STACK_CS_OFFSET = 17;
const unsigned int STACK_RFLAGS_OFFSET = 18;
const unsigned int STACK_RSP_OFFSET = 19;
const unsigned int STACK_SS_OFFSET = 20;
const unsigned int STACK_RCX_OFFSET = 12;

const unsigned long DEF_RFLAGS_KERNEL = (unsigned long)0x0200;
const unsigned long DEF_CS_KERNEL = 0x08;
const unsigned long DEF_SS_KERNEL = 0x10;

const unsigned long DEF_RFLAGS_USER = (unsigned long)0x3200;
const unsigned long DEF_CS_USER = 0x18;
const unsigned long DEF_SS_USER = 0x20;

/// @brief Create a new x64 execution context
///
/// Create an entire x64 execution context that will cause entry_point to be executed within new_thread. This must only
/// be called once for each thread object.
///
/// @param entry_point The point where the new thread will begin executing
///
/// @param new_thread The thread that is having an execution context created for it
///
/// @return A pointer to the execution context. This is opaque to non-x64 code.
void *task_int_create_exec_context(ENTRY_PROC entry_point, task_thread *new_thread)
{
  task_x64_exec_context *new_context;
  unsigned long *stack_addr;
  task_process *parent_process;
  mem_process_info *memmgr_data;
  process_x64_data *memmgr_x64_data;
  unsigned long new_flags;
  unsigned long new_cs;
  unsigned long new_ss;

  KL_TRC_ENTRY;

  ASSERT(new_thread != nullptr);
  parent_process = new_thread->parent_process;
  ASSERT(parent_process != nullptr);
  memmgr_data = parent_process->mem_info;
  ASSERT(memmgr_data != nullptr);
  memmgr_x64_data = (process_x64_data *)memmgr_data->arch_specific_data;
  ASSERT(memmgr_data != nullptr);

  // Fill in the easy parts of the context
  new_context = new task_x64_exec_context;
  new_context->exec_ptr = (void *)entry_point;
  KL_TRC_DATA("Exec pointer", (unsigned long)entry_point);
  new_context->cr3_value = (void *)memmgr_x64_data->pml4_phys_addr;
  KL_TRC_DATA("CR3", (unsigned long)new_context->cr3_value);

  // Allocate a whole 2MB for the stack. The stack grows downwards, so position the expected stack pointer near to the
  // end of the page.
  stack_addr = static_cast<unsigned long *>(proc_x64_allocate_stack());
  stack_addr -= req_stack_entries;
  KL_TRC_DATA("Stack address", (unsigned long)stack_addr);

  if (parent_process->kernel_mode)
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "Creating a kernel mode exec context.\n"));
    new_flags = DEF_RFLAGS_KERNEL;
    new_cs = DEF_CS_KERNEL;
    new_ss = DEF_SS_KERNEL;
  }
  else
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "Creating a user mode exec context.\n"));
    new_flags = DEF_RFLAGS_USER;
    // The +3s below add the RPL (ring 3) to the CS/SS selectors, which don't otherwise contain them.
    new_cs = DEF_CS_USER + 3;
    new_ss = DEF_SS_USER + 3;
  }

  stack_addr[STACK_EFLAGS_OFFSET] = new_flags;
  stack_addr[STACK_RIP_OFFSET] = (unsigned long)entry_point;
  stack_addr[STACK_CS_OFFSET] = new_cs;
  stack_addr[STACK_RFLAGS_OFFSET] = new_flags;
  stack_addr[STACK_RSP_OFFSET] = (unsigned long)stack_addr + (req_stack_entries * sizeof(unsigned long));
  stack_addr[STACK_SS_OFFSET] = new_ss;
  stack_addr[STACK_RCX_OFFSET] = 0x1234;

  new_context->stack_ptr = (void *)stack_addr;

  KL_TRC_EXIT;
  return (void *)new_context;
}

// TODO: Note that at present, exec_ptr is always 0 because RIP is stored in the stack.
// TODO: Probably should remove it then...
/// @brief Main task switcher
///
/// task_int_swap_task() is called by the timer interrupt. It saves the execution context of the thread currently
/// executing, selects the next one and provides the new execution context to the caller.
///
/// The action of choosing the next thread to execute is not platform specific, it is provided by generic code in
/// #task_get_next_thread.
///
/// @param stack_ptr The stack pointer that provides the execution context that has just finished executing
///
/// @param exec_ptr The value of the program counter for the suspended thread (note: This is not actually used, or even
///                 correct at the moment.
///
/// @param cr3_value The value of CR3 used by the suspended thread
///
/// @return The execution context for the caller to begin executing.
task_x64_exec_context *task_int_swap_task(unsigned long stack_ptr, unsigned long exec_ptr, unsigned long cr3_value)
{
  task_thread *current_thread;
  task_x64_exec_context *current_context;
  task_x64_exec_context *next_context;
  task_thread *next_thread;

  KL_TRC_ENTRY;

  current_thread = task_get_cur_thread();
  KL_TRC_DATA("Current thread", (unsigned long)current_thread);
  if (current_thread != nullptr)
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "Storing exec context\n"));
    current_context = (task_x64_exec_context *)current_thread->execution_context;
    current_context->stack_ptr = (void *)stack_ptr;
    current_context->exec_ptr = (void *)exec_ptr;
    current_context->cr3_value = (void *)cr3_value;
    KL_TRC_DATA("Storing CR3", (unsigned long)current_context->cr3_value);
    KL_TRC_DATA("Storing Stack pointer", (unsigned long)current_context->stack_ptr);
    KL_TRC_DATA("Computed RIP of leaving thread", *(((unsigned long *)current_context->stack_ptr)+STACK_RIP_OFFSET));
  }

  next_thread = task_get_next_thread();
  KL_TRC_DATA("Next thread", (unsigned long)next_thread);
  next_context = (task_x64_exec_context *)next_thread->execution_context;

  KL_TRC_DATA("New CR3 value", (unsigned long)next_context->cr3_value);
  KL_TRC_DATA("Stack pointer", (unsigned long)next_context->stack_ptr);
  KL_TRC_DATA("Computed RIP of new thread", *(((unsigned long *)next_context->stack_ptr)+STACK_RIP_OFFSET));

  // Only processor 0 directly receives timer interrupts. In order to trigger scheduling on all other processors, send
  // them an IPI for the correct vector.
  if (proc_mp_this_proc_id() == 0)
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "Sending broadcast IPI\n"));
    proc_send_ipi(0, PROC_IPI_SHORT_TARGET::ALL_EXCL_SELF, PROC_IPI_INTERRUPT::FIXED, 32);
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

  proc_configure_idt_entry(32, 0, (void *)asm_task_switch_interrupt, 0);
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
