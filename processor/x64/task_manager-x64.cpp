//#define ENABLE_TRACING

#include "klib/klib.h"

#include "processor/processor.h"
#include "processor/processor-int.h"
#include "processor/x64/processor-x64-int.h"
#include "mem/x64/mem_internal_x64.h"

// Use this to get the address of the kernel's page tables.
// TODO: Make sure this gets removed when setting up multiple processes (MT)
// TODO: Looks like it's already been done? (19/07/16)
//extern unsigned long pml4_table;


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

  ASSERT(new_thread != NULL);
  parent_process = new_thread->parent_process;
  ASSERT(parent_process != NULL);
  memmgr_data = parent_process->mem_info;
  ASSERT(memmgr_data != NULL);
  memmgr_x64_data = (process_x64_data *)memmgr_data->arch_specific_data;
  ASSERT(memmgr_data != NULL);

  // Fill in the easy parts of the context
  new_context = new task_x64_exec_context;
  new_context->exec_ptr = (void *)entry_point;
  KL_TRC_DATA("Exec pointer", (unsigned long)entry_point);
  new_context->cr3_value = (void *)memmgr_x64_data->pml4_phys_addr;
  KL_TRC_DATA("CR3", (unsigned long)new_context->cr3_value);

  // Allocate a whole 2MB for the stack. The stack grows downwards, so position the expected stack pointer near to the
  // end of the page.
  stack_addr = (unsigned long *)kmalloc(MEM_PAGE_SIZE);
  stack_addr += (MEM_PAGE_SIZE / sizeof(unsigned long));
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
task_x64_exec_context *task_int_swap_task(unsigned long stack_ptr, unsigned long exec_ptr, unsigned long cr3_value)
{
  task_thread *current_thread;
  task_x64_exec_context *current_context;
  task_x64_exec_context *next_context;

  KL_TRC_ENTRY;

  current_thread = task_get_cur_thread();
  if (current_thread != NULL)
  {
    current_context = (task_x64_exec_context *)current_thread->execution_context;
    current_context->stack_ptr = (void *)stack_ptr;
    current_context->exec_ptr = (void *)exec_ptr;
    current_context->cr3_value = (void *)cr3_value;
    KL_TRC_DATA("Storing CR3", (unsigned long)current_context->cr3_value);
    KL_TRC_DATA("Storing Stack pointer", (unsigned long)current_context->stack_ptr);
    KL_TRC_DATA("Computed RIP of leaving thread", *(((unsigned long *)current_context->stack_ptr)+STACK_RIP_OFFSET));
  }

  next_context = (task_x64_exec_context *)task_get_next_thread()->execution_context;
  KL_TRC_DATA("New CR3 value", (unsigned long)next_context->cr3_value);
  KL_TRC_DATA("Stack pointer", (unsigned long)next_context->stack_ptr);
  KL_TRC_DATA("Computed RIP of new thread", *(((unsigned long *)next_context->stack_ptr)+STACK_RIP_OFFSET));

  KL_TRC_EXIT;

  return next_context;
}

// Sets up the timer interrupt to call the task switching mechanism.
void task_install_task_switcher()
{
  KL_TRC_ENTRY;

  // TODO: Do a more sensible redirection than this!
  proc_configure_idt_entry(34, 0, (void *)asm_task_switch_interrupt);
  asm_proc_install_idt();

  KL_TRC_EXIT;
}

// Platform-specific initialization required before tasking can start.
// On x64, this is to create a suitable TSS and TSS descriptor.
void task_platform_init()
{
  KL_TRC_ENTRY;

  proc_init_tss();

  KL_TRC_EXIT;
}
