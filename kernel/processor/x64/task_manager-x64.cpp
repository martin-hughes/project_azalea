/// @file
/// @brief x64-specific part of the task manager

//#define ENABLE_TRACING

#include "klib/klib.h"

#include "processor/processor.h"
#include "processor/processor-int.h"
#include "processor/x64/processor-x64.h"
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

const unsigned int TM_INTERRUPT_NUM = IRQ_BASE;

const unsigned long DEF_USER_MODE_STACK_PAGE = 0x000000F000000000;

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

  // The address of the stack for this process, as seen by the kernel.
  unsigned long *kernel_stack_addr;

  // The value about to go into RSP when the process is running. (That is, for a user-mode process, this will be a user
  // mode address, not a kernel mode one.)
  unsigned long rsp_of_stack;
  unsigned long working_addr;

  task_process *parent_process;
  mem_process_info *memmgr_data;
  process_x64_data *memmgr_x64_data;
  unsigned long new_flags;
  unsigned long new_cs;
  unsigned long new_ss;
  void *kernel_stack_page = nullptr;

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
  if (parent_process->kernel_mode)
  {
    // The kernel has a fairly well-defined way of generating stacks for itself, no need to fiddle with page maps.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Creating a kernel mode exec context.\n");
    new_flags = DEF_RFLAGS_KERNEL;
    new_cs = DEF_CS_KERNEL;
    new_ss = DEF_SS_KERNEL;

    kernel_stack_addr = static_cast<unsigned long *>(proc_x64_allocate_stack());
    rsp_of_stack = reinterpret_cast<unsigned long>(kernel_stack_addr);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Creating a user mode exec context.\n");
    new_flags = DEF_RFLAGS_USER;
    // The +3s below add the RPL (ring 3) to the CS/SS selectors, which don't otherwise contain them.
    new_cs = DEF_CS_USER + 3;
    new_ss = DEF_SS_USER + 3;

    // We need the user mode process's stack to be visible to it in user-mode, but we also need to be able to access it
    // now. So, allocate a single page, but map it in both kernel and user space.
    kernel_stack_page = mem_allocate_virtual_range(1);
    unsigned long ksp_ulong = reinterpret_cast<unsigned long>(kernel_stack_page);
    void *physical_backing = mem_allocate_physical_pages(1);
    mem_map_range(physical_backing, kernel_stack_page, 1);
    mem_map_range(physical_backing, reinterpret_cast<void *>(DEF_USER_MODE_STACK_PAGE), 1, parent_process);

    // For the time being, all user-mode processes start with the same stack address. It's important that the offset
    // within the page is kept the same for both user and kernel addresses.
    kernel_stack_addr = reinterpret_cast<unsigned long *>(ksp_ulong + MEM_PAGE_SIZE - 8);
    rsp_of_stack = reinterpret_cast<unsigned long>(DEF_USER_MODE_STACK_PAGE + MEM_PAGE_SIZE - 8);
  }

  // Fill in the execution context part of the stack.
  // As above, both kernel and (potentially) user mode versions of the stack address need offsetting by the same
  // amount. These look different because kernel_stack_addr is a pointer, but rsp_of_stack is not. The effect is the
  // same for both though.
  kernel_stack_addr -= req_stack_entries;
  rsp_of_stack -= (req_stack_entries * sizeof(unsigned long));
  kernel_stack_addr[STACK_EFLAGS_OFFSET] = new_flags;
  kernel_stack_addr[STACK_RIP_OFFSET] = (unsigned long)entry_point;
  kernel_stack_addr[STACK_CS_OFFSET] = new_cs;
  kernel_stack_addr[STACK_RFLAGS_OFFSET] = new_flags;
  kernel_stack_addr[STACK_RSP_OFFSET] = rsp_of_stack + (req_stack_entries * sizeof(unsigned long));
  kernel_stack_addr[STACK_SS_OFFSET] = new_ss;
  kernel_stack_addr[STACK_RCX_OFFSET] = 0x1234;

  // Move the rsp_of_stack pointer down to provide space for the FPU storage space. It is moved down by 512 bytes,
  // and then rounded down to the next 16-byte boundary. rsp_of_stack doesn't need to change, since we store it later.
  working_addr = reinterpret_cast<unsigned long>(kernel_stack_addr);
  working_addr -= 512;
  working_addr = working_addr & 0xFFFFFFFFFFFFFFF0;

  // Simply clear the FPU storage space
  kl_memset(reinterpret_cast<void *>(working_addr), 0, 512);

  // Move the stack 16-bytes further down. The next value in the stack is the stack pointer above the FPU storage
  // space - it may be 8-byte aligned. Then there's another 8 bytes of blank space.
  working_addr -= 16;
  kernel_stack_addr = reinterpret_cast<unsigned long *>(working_addr);
  *kernel_stack_addr = rsp_of_stack;

  if (!parent_process->kernel_mode)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unmapping kernel addresses of user-mode stack\n");
    ASSERT(kernel_stack_page != nullptr);
    mem_unmap_range(kernel_stack_page, 1);
  }

  new_context->stack_ptr = (void *)((rsp_of_stack - 528) & 0xFFFFFFFFFFFFFFF0);

  KL_TRC_TRACE(TRC_LVL::EXTRA,  "Final kernel stack address", kernel_stack_addr, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA,  "Final RSP address", rsp_of_stack, "\n");

  KL_TRC_EXIT;
  return (void *)new_context;
}

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
/// @param exec_ptr The value of the program counter for the suspended thread. Note: This value is for information
///                 only - When the thread resumes, it will use the value of RIP that is stored in the stack above
///                 stack_ptr.
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
    KL_TRC_TRACE(TRC_LVL::FLOW, "Storing exec context\n");
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

  // Save the thread structure's address in IA32_KERNEL_GS_BASE in order that the processor can uniquely identify the
  // thread without having to look in a list (which is subject to threads moving between processors whilst looking in
  // the list.
  proc_write_msr(PROC_X64_MSRS::IA32_KERNEL_GS_BASE, reinterpret_cast<unsigned long>(next_thread));

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

  proc_configure_idt_entry(TM_INTERRUPT_NUM, 0, (void *)asm_task_switch_interrupt, 0);
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
  // (which might be this one)
  proc_send_ipi(0, PROC_IPI_SHORT_TARGET::SELF, PROC_IPI_INTERRUPT::FIXED, TM_INTERRUPT_NUM, true);

  KL_TRC_EXIT;
}

/// @brief Return pointer to the currently executing thread
///
/// @return The task_thread object of the executing thread on this processor, or NULL if we haven't got that far yet.
task_thread *task_get_cur_thread()
{
  KL_TRC_ENTRY;

  unsigned int proc_id;

  task_thread *ret_thread;
  ret_thread = reinterpret_cast<task_thread *>(proc_read_msr(PROC_X64_MSRS::IA32_KERNEL_GS_BASE));

  KL_TRC_EXIT;

  return ret_thread;
}
