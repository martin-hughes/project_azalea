// Project Azalea Kernel
// Main entry point.

#define ENABLE_TRACING

#include <stdint.h>

#include "klib/klib.h"
#include "processor/processor.h"
#include "processor/timing/timing.h"
#include "mem/mem.h"
#include "syscall/syscall_kernel.h"
#include "acpi/acpi_if.h"
#include "object_mgr/object_mgr.h"
#include "system_tree/system_tree.h"
#include "system_tree/process/process.h"

#include "devices/block/ata/ata.h"
#include "devices/block/proxy/block_proxy.h"
#include "devices/legacy/ps2/ps2_controller.h"
#include "system_tree/fs/fat/fat_fs.h"
#include "system_tree/fs/pipe/pipe_fs.h"

#include "entry/multiboot.h"

#include <memory>

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
// - Permit full ACPI.
// - Load the user-mode "init" task (currently done by temporary code)

extern "C" int main(unsigned int magic_number, multiboot_hdr *mb_header);
void kernel_start();
void simple_terminal();
void setup_task_parameters(task_process *startup_proc);

// Temporary procedures and storage while the kernel is being developed. Eventually, the full kernel start procedure
// will cause these to become unused.
void setup_initial_fs();
// Some variables to support loading a filesystem.
generic_ata_device *first_hdd;
fat_filesystem *first_fs;
gen_ps2_controller_device *ps2_controller;

volatile bool wait_for_term;

// Assumptions used throughout the kernel
static_assert(sizeof(unsigned int) == 4, "Unsigned long assumed to be length 4.");
static_assert(sizeof(uint64_t) == sizeof(uintptr_t), "Code throughout assumes pointers are 64-bits long.");
static_assert(sizeof(unsigned long) == 8, "Unsigned long must be 8 bytes");

// Main kernel entry point. This is called by an assembly-language loader that should do as little as possible. On x64,
// this involves setting up a simple page mapping, since the kernel is linked higher-half but loaded at 1MB, then
// kicking the task manager in to life.
int main(uint32_t magic_number, multiboot_hdr *mb_header)
{
  // The kernel needs the information table provided by the multiboot loader in order to function properly.
  if (magic_number != MULTIBOOT_CONSTANT)
  {
    panic("Not booted by a multiboot compliant loader");
  }
  ASSERT(mb_header != nullptr);
  // Check that the memory map flag is set.
  ASSERT((mb_header->flags && (1 << 6)) != 0);

  // Gather details about the memory map in advance of giving them to the memory manager.
  uint64_t e820_map_addr = mb_header->mmap_addr;
  e820_pointer e820_ptr;
  e820_ptr.table_ptr = reinterpret_cast<e820_record *>(e820_map_addr);
  e820_ptr.table_length = mb_header->mmap_length;

  proc_gen_init();
  mem_gen_init(&e820_ptr);
  hm_gen_init();
  om_gen_init();
  system_tree_init();
  acpi_init_table_system();
  time_gen_init();
  proc_mp_init();
  syscall_gen_init();
  task_init();

  KL_TRC_TRACE(TRC_LVL::IMPORTANT, "Welcome to the OS!\n");

  task_process *kernel_process = task_create_new_process(kernel_start, true, mem_task_get_task0_entry());
  task_start_process(kernel_process);

  task_start_tasking();

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
               reinterpret_cast<uint64_t>(task_get_cur_thread()),
               "\n");

  ACPI_STATUS status;
  task_process *initial_proc;

  // Bring the ACPI system up to full readiness.
  status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
  ASSERT(status == AE_OK);

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Code below here is not intended to be part of the permanent kernel start procedure, but will sit here until the //
  // kernel is more well-developed.                                                                                  //
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // Enable the PS/2 controller.
  ps2_controller = new gen_ps2_controller_device();

  // Setup a basic file system.
  setup_initial_fs();
  ASSERT(first_fs != nullptr);
  ASSERT(system_tree()->add_branch("root", first_fs) == ERR_CODE::NO_ERROR);

  wait_for_term = true;

  initial_proc = proc_load_elf_file("root\\initprog");
  setup_task_parameters(initial_proc);
  ASSERT(initial_proc != nullptr);

  // Start a simple terminal process.
  task_process *term = task_create_new_process(simple_terminal, true);
  KL_TRC_TRACE(TRC_LVL::FLOW, "Starting terminal\n");
  task_start_process(term);

  ps2_keyboard_device *keyboard = dynamic_cast<ps2_keyboard_device *>(ps2_controller->chan_1_dev);
  ASSERT(keyboard != nullptr);

  while (wait_for_term)
  {

  }

  // Process should be good to go!
  task_start_process(initial_proc);

  if (keyboard != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Setting up keyboard messages\n");
    keyboard->recipient = initial_proc;
  }

  // If (when!) the initial process exits, we want the system to shut down. But since we don't really do shutting down
  // at the moment, just crash instead.
  initial_proc->wait_for_signal();

  panic("System has 'shut down'");
}

// Configure the filesystem of the (presumed) boot device as part of System Tree.
const unsigned int base_reg_a = 0x1F0;
void setup_initial_fs()
{
  KL_TRC_ENTRY;

  first_hdd = new generic_ata_device(base_reg_a, true);
  std::unique_ptr<unsigned char[]> sector_buffer(new unsigned char[512]);

  kl_memset(sector_buffer.get(), 0, 512);
  if (first_hdd->read_blocks(0, 1, sector_buffer.get(), 512) != ERR_CODE::NO_ERROR)
  {
    panic("Disk read failed :(\n");
  }

  // Confirm that we've loaded a valid MBR
  KL_TRC_TRACE(TRC_LVL::EXTRA, (uint64_t)sector_buffer[510], " ", (uint64_t)sector_buffer[511], "\n");
  ASSERT((sector_buffer[510] == 0x55) && (sector_buffer[511] == 0xAA));

  uint32_t start_sector;
  uint32_t sector_count;

  // Parse the MBR to find the first partition.
  kl_memcpy(sector_buffer.get() + 454, &start_sector, 4);
  kl_memcpy(sector_buffer.get() + 458, &sector_count, 4);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "First partition: ", (uint64_t)start_sector, " -> +", (uint64_t)sector_count, "\n");
  block_proxy_device *pd = new block_proxy_device(first_hdd, start_sector, sector_count);

  // Initialise the filesystem based on that information
  first_fs = new fat_filesystem(pd);

  KL_TRC_EXIT;
}

// A simple text based terminal outputting on the main display.
void simple_terminal()
{
  KL_TRC_ENTRY;

  ISystemTreeLeaf *leaf;
  IReadable *reader;
  const uint64_t buffer_size = 10;
  unsigned char buffer[buffer_size];
  uint64_t bytes_read;
  unsigned char *display_ptr;

  const uint16_t width = 80;
  const uint16_t height = 25;
  const uint16_t bytes_per_char = 2;

  uint16_t cur_offset = 0;

  // Set up the input pipe
  ISystemTreeBranch *pipes_br = new system_tree_simple_branch();
  ASSERT(pipes_br != nullptr);
  ASSERT(system_tree() != nullptr);
  ASSERT(system_tree()->add_branch("pipes", pipes_br) == ERR_CODE::NO_ERROR);
  ASSERT(pipes_br->add_branch("terminal", new pipe_branch()) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->get_leaf("pipes\\terminal\\read", &leaf) == ERR_CODE::NO_ERROR);
  reader = dynamic_cast<IReadable *>(leaf);
  ASSERT(reader != nullptr);

  // Map and then clear the display
  display_ptr = reinterpret_cast<unsigned char *>(mem_allocate_virtual_range(1));
  mem_map_range(nullptr, display_ptr, 1);
  display_ptr += 0xB8000;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Clearing screen\n");
  for (int i = 0; i < width * height * bytes_per_char; i += 2)
  {
    display_ptr[i] = 0;
    display_ptr[i + 1] = 0x0f;
  }

  wait_for_term = false;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Beginning terminal\n");
  while(1)
  {
    // Write any pending output data.
    if (reader->read_bytes(0, buffer_size, buffer, buffer_size, bytes_read) == ERR_CODE::NO_ERROR)
    {
      for (int i = 0; i < bytes_read; i++)
      {
        display_ptr[cur_offset * 2] = buffer[i];

        cur_offset++;
        if (cur_offset > (width * height))
        {
          cur_offset = 0;
        }
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to read\n");
    }
  }

  KL_TRC_EXIT;
}

// Setup a plausible argc, argv and environ in startup_proc.
// Let's go for:
// argc = 2
// argv = "initprog", "testparam"
// environ = "OSTYPE=azalea"
void setup_task_parameters(task_process *startup_proc)
{
  // The default user mode stack starts from this position - 16 and grows downwards, we put the task parameters above
  // this position.
  const unsigned long default_posn = 0x000000000F200000;

  KL_TRC_ENTRY;

  void *physical_backing;
  void *kernel_map;
  char **argv_ptr_k;
  char **environ_ptr_k;
  char *string_ptr_k;

  char **argv_ptr_u;
  char **environ_ptr_u;
  char *string_ptr_u;

  ASSERT(startup_proc != nullptr);
  ASSERT(mem_get_phys_addr(reinterpret_cast<void *>(default_posn)) == nullptr);

  physical_backing = mem_allocate_physical_pages(1);
  kernel_map = mem_allocate_virtual_range(1);

  mem_map_range(physical_backing, kernel_map, 1);
  mem_vmm_allocate_specific_range(default_posn, 1, startup_proc);
  mem_map_range(physical_backing, reinterpret_cast<void *>(default_posn), 1, startup_proc);

  argv_ptr_k = reinterpret_cast<char **>(kernel_map);
  argv_ptr_u = reinterpret_cast<char **>(default_posn);
  // The end of argv is nullptr.
  argv_ptr_k[2] = nullptr;

  string_ptr_k = reinterpret_cast<char *>(argv_ptr_k + 3);
  string_ptr_u = reinterpret_cast<char *>(argv_ptr_u + 3);

  argv_ptr_k[0] = string_ptr_u;
  kl_memcpy("initprog", string_ptr_k, 9);
  string_ptr_k += 9;
  string_ptr_u += 9;

  argv_ptr_k[1] = string_ptr_u;
  kl_memcpy("testparam", string_ptr_k, 10);
  string_ptr_k += 10;
  string_ptr_u += 10;

  environ_ptr_k = reinterpret_cast<char **>(reinterpret_cast<unsigned long>(kernel_map) + 64);
  environ_ptr_u = reinterpret_cast<char **>(default_posn + 64);
  environ_ptr_k[1] = nullptr;
  string_ptr_k = reinterpret_cast<char *>(environ_ptr_k + 2);
  string_ptr_u = reinterpret_cast<char *>(environ_ptr_u + 2);
  environ_ptr_k[0] = string_ptr_u;

  kl_memcpy("OSTYPE=azalea", string_ptr_k, 14);

  task_set_start_params(startup_proc, 2, argv_ptr_u, environ_ptr_u);

  mem_unmap_range(kernel_map, 1);

  KL_TRC_EXIT;
}