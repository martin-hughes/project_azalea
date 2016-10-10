// Project Azalea Kernel
// Main entry point.

#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor/processor.h"
#include "processor/timing/timing.h"
#include "mem/mem.h"
#include "syscall/syscall_kernel.h"
#include "acpi/acpi_if.h"
#include "object_mgr/object_mgr.h"
#include "system_tree/system_tree.h"

// Rough boot steps:
//
// main() function:
// - Initialise main processor. (For x64, this is GDT, IDT)
// - Initialise memory manager.
// - Initialise kernel data stores. (HM, OM, ST)
// - Initialise other processors, but leave them suspended.
// - Prepare the system call interface on all processors.
// - Initialise the task manager with the kernel's wake-up task (which is in ring 0)
// - Start the clock, so that task is kicked in to life.
//
// Kernel wake-up task (kernel_start()):
// - Bring other processors in to the task scheduling loop

void kernel_start();
//void dummy_proc();

// This one calls int 49
//const unsigned char dummy_prog[] = { 0xCD, 0x31, 0xB8, 0x02, 0x00, 0x00, 0x00, 0xEB, 0xF7 };

// This one just loops.
//const unsigned char dummy_prog[] = { 0xB8, 0x02, 0x00, 0x00, 0x00, 0xEB, 0xF9 };

// This one calls syscall
const unsigned char dummy_prog[] = { 0x0F, 0x05, 0x0F, 0x05, 0x0F, 0x05, 0xB8, 0x02, 0x00, 0x00, 0x00, 0x0F, 0x05, 0xEB, 0xF7 };

typedef void (*fn_ptr)();

// Main kernel entry point. This is called by an assembly-language loader that
// should do as little as possible. On x64, this involves setting up a simple
// page mapping, since the kernel is linked higher-half but loaded at 1MB, then kicking the task manager in to life.
int main()
{
  proc_gen_init();
  mem_gen_init();
  hm_gen_init();
  om_gen_init();
  system_tree_init();

  KL_TRC_TRACE(TRC_LVL::IMPORTANT, "Welcome to the OS!\n");

  acpi_init_table_system();

  proc_mp_init();
  syscall_gen_init();

  time_gen_init();
  task_gen_init(kernel_start);

  // If the kernel gets back to here, just run in a loop. The task manager will soon kick in.
  // It takes too long, then assume something has gone wrong and abort.
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, "Back to main(), waiting for start.\n");
  time_stall_process(1000000000);

  panic("System failed to start - main timer hasn't hit.");

  proc_stop_all_procs();
  return (0);
};

// Main kernel start procedure.
void kernel_start()
{
  KL_TRC_TRACE(TRC_LVL::FLOW,
               "Entered kernel start - thread: ",
               reinterpret_cast<unsigned long>(task_get_cur_thread()),
               "\n");

  ACPI_STATUS status;

  // kernel_start() runs on the BSP. Bring up the APs so they are ready to take on any threads created below.
  proc_mp_start_aps();

  // Bring the ACPI system up to full readiness.
  status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
  ASSERT(status == AE_OK);

  //task_create_new_process(dummy_proc, false);

  // Create a new user mode process.
  fn_ptr user_proc = (fn_ptr)0x200000;
  task_process *new_proc;
  new_proc = task_create_new_process(user_proc, false);
  ASSERT(new_proc != nullptr);

  // Allocate it some memory for the code to go in, and allow the kernel to access it.
  void *physical_page = mem_allocate_physical_pages(1);
  void *kernel_virtual_page = mem_allocate_virtual_range(1);
  KL_TRC_DATA("Physical page to use", (unsigned long)physical_page);
  KL_TRC_DATA("Kernel virtual page", (unsigned long)kernel_virtual_page);
  mem_map_range(physical_page, kernel_virtual_page, 1);
  KL_TRC_TRACE(TRC_LVL::FLOW, "First map complete\n");

  // Copy the simple test program in to it.
  kl_memcpy((void *)dummy_prog, kernel_virtual_page, sizeof(dummy_prog));
  KL_TRC_TRACE(TRC_LVL::FLOW, "Program copied\n");

  // No need to access it from the kernel any more
  mem_unmap_range(kernel_virtual_page, 1);
  KL_TRC_TRACE(TRC_LVL::FLOW, "Kernel space unmapped\n");

  // In the context of the program, set up the virtual page allocation. The process starts at 2MB.
  mem_map_range(physical_page, (void *)0x200000, 1, new_proc);
  KL_TRC_TRACE(TRC_LVL::FLOW, "User mode map complete, starting now.\n");

  // Process should be good to go!
  task_start_process(new_proc);

  while (1)
  {
    //Spin forever.
  }
}

/*void dummy_proc()
{
  KL_TRC_TRACE((TRC_LVL_FLOW, "Entered dummy proc\n"));
  while (1)
  {
    //Spin forever.
  }
}*/
