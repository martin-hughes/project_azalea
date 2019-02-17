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
#include "devices/generic/gen_terminal.h"
#include "system_tree/fs/fat/fat_fs.h"
#include "system_tree/fs/pipe/pipe_fs.h"
#include "system_tree/fs/mem/mem_fs.h"
#include "system_tree/fs/dev/dev_fs.h"

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

// Known deficiencies:
// Where to begin!
// - The mapping of a pipe leaf to the process stdout is sketchy, at best. It will be improved once a bit more work is
//   done on loading processes.

extern "C" int main(unsigned int magic_number, multiboot_hdr *mb_header);
void kernel_start() throw ();
void setup_task_parameters(task_process *startup_proc);

// Temporary procedures and storage while the kernel is being developed. Eventually, the full kernel start procedure
// will cause these to become unused.
std::shared_ptr<fat_filesystem> setup_initial_fs();
// Some variables to support loading a filesystem.
generic_ata_device *first_hdd;
gen_ps2_controller_device *ps2_controller;

std::shared_ptr<task_process> *system_process;
std::shared_ptr<task_process> *kernel_start_process;

extern task_process *term_proc;
task_process *term_proc = nullptr;

volatile bool wait_for_term;

// Assumptions used throughout the kernel
static_assert(sizeof(uint64_t) == sizeof(uintptr_t), "Code throughout assumes pointers are 64-bits long.");

// There are a few places to check before this assert can be removed - ACPI headers for example.
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
  system_tree_init();
  acpi_init_table_system();
  time_gen_init();
  proc_mp_init();
  syscall_gen_init();

  system_process = new std::shared_ptr<task_process>();
  kernel_start_process = new std::shared_ptr<task_process>();

  *system_process = task_init();

  KL_TRC_TRACE(TRC_LVL::IMPORTANT, "Welcome to the OS!\n");

  *kernel_start_process = task_process::create(kernel_start, true, mem_task_get_task0_entry());
  (*kernel_start_process)->start_process();

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
void kernel_start() throw ()
{
  KL_TRC_TRACE(TRC_LVL::FLOW,
               "Entered kernel start - thread: ",
               reinterpret_cast<uint64_t>(task_get_cur_thread()),
               "\n");

  ACPI_STATUS status;
  std::shared_ptr<task_process> initial_proc;
  std::shared_ptr<ISystemTreeLeaf> leaf;
  char proc_ptr_buffer[34];
  const char hello_string[] = "Hello, world!";
  uint64_t br;
  std::shared_ptr<pipe_branch::pipe_read_leaf> pipe_read_leaf;

  // Bring the ACPI system up to full readiness.
  status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
  ASSERT(status == AE_OK);

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Code below here is not intended to be part of the permanent kernel start procedure, but will sit here until the //
  // kernel is more well-developed.                                                                                  //
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // Start the device management system.
  std::shared_ptr<dev_root_branch> dev_root = std::make_shared<dev_root_branch>();
  ASSERT(system_tree()->add_child("dev", dev_root) == ERR_CODE::NO_ERROR);
  dev_root->scan_for_devices();

  // Enable the PS/2 controller.
  ps2_controller = new gen_ps2_controller_device();

  // Setup a basic file system.
  std::shared_ptr<fat_filesystem> first_fs = setup_initial_fs();
  ASSERT(first_fs != nullptr);
  ASSERT(system_tree()->add_child("root", std::dynamic_pointer_cast<ISystemTreeBranch>(first_fs)) == ERR_CODE::NO_ERROR);

  wait_for_term = true;

  initial_proc = proc_load_elf_file("root\\initprog");
  setup_task_parameters(initial_proc.get());
  ASSERT(initial_proc != nullptr);

  // Create a temporary in-RAM file system.
  std::shared_ptr<mem_fs_branch> ram_branch = mem_fs_branch::create();
  ASSERT(ram_branch != nullptr);
  ASSERT(system_tree()->add_child("temp", ram_branch) == ERR_CODE::NO_ERROR);
  std::shared_ptr<mem_fs_leaf> ram_file = std::make_shared<mem_fs_leaf>(ram_branch);
  ASSERT(ram_file != nullptr);
  ASSERT(system_tree()->add_child("temp\\hello.txt", ram_file) == ERR_CODE::NO_ERROR);
  ASSERT(ram_file->write_bytes(0,
                               kl_strlen(hello_string, sizeof(hello_string)),
                               reinterpret_cast<const uint8_t *>(hello_string),
                               sizeof(hello_string), br) == ERR_CODE::NO_ERROR);
  ASSERT(br == sizeof(hello_string) - 1);

  // Start a simple terminal process.
  std::shared_ptr<task_process> term = task_process::create(simple_terminal, true);
  KL_TRC_TRACE(TRC_LVL::FLOW, "Starting terminal\n");
  term->start_process();

  ps2_keyboard_device *keyboard = dynamic_cast<ps2_keyboard_device *>(ps2_controller->chan_1_dev);
  ASSERT(keyboard != nullptr);

  while (wait_for_term)
  {

  }

  // Setup the write end of the terminal pipe. This is a bit dubious, it doesn't do any reference counting...
  ASSERT(system_tree()->get_child("pipes\\terminal-output\\write", leaf) == ERR_CODE::NO_ERROR);
  klib_snprintf(proc_ptr_buffer, 34, "proc\\%p\\stdout", initial_proc.get());

  KL_TRC_TRACE(TRC_LVL::FLOW, "proc: ", (const char *)proc_ptr_buffer, "\n");
  ASSERT(system_tree()->add_child(proc_ptr_buffer, leaf) == ERR_CODE::NO_ERROR);

  klib_snprintf(proc_ptr_buffer, 34, "proc\\%p\\stdin", initial_proc.get());
  ASSERT(system_tree()->get_child("pipes\\terminal-input\\read", leaf) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->add_child(proc_ptr_buffer, leaf) == ERR_CODE::NO_ERROR);
  pipe_read_leaf = std::dynamic_pointer_cast<pipe_branch::pipe_read_leaf>(leaf);
  ASSERT(pipe_read_leaf != nullptr);
  pipe_read_leaf->set_block_on_read(true);

  // Process should be good to go!
  initial_proc->start_process();
  term_proc = term.get();

  // If (when!) the initial process exits, we want the system to shut down. But since we don't really do shutting down
  // at the moment, just crash instead.
  initial_proc->wait_for_signal();

  panic("System has 'shut down'");
}

// Configure the filesystem of the (presumed) boot device as part of System Tree.
const unsigned int base_reg_a = 0x1F0;
std::shared_ptr<fat_filesystem> setup_initial_fs()
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
  std::shared_ptr<block_proxy_device> pd = std::make_shared<block_proxy_device>(first_hdd, start_sector, sector_count);

  // Initialise the filesystem based on that information
  std::shared_ptr<fat_filesystem> first_fs = fat_filesystem::create(pd);

  KL_TRC_EXIT;
  return first_fs;
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
  const uintptr_t default_posn = 0x000000000F200000;

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

  environ_ptr_k = reinterpret_cast<char **>(reinterpret_cast<uint64_t>(kernel_map) + 64);
  environ_ptr_u = reinterpret_cast<char **>(default_posn + 64);
  environ_ptr_k[1] = nullptr;
  string_ptr_k = reinterpret_cast<char *>(environ_ptr_k + 2);
  string_ptr_u = reinterpret_cast<char *>(environ_ptr_u + 2);
  environ_ptr_k[0] = string_ptr_u;

  kl_memcpy("OSTYPE=azalea", string_ptr_k, 14);

  task_set_start_params(startup_proc, 2, argv_ptr_u, environ_ptr_u);

  mem_unmap_range(kernel_map, 1, nullptr, false);

  KL_TRC_EXIT;
}