/// @file
/// @brief Project Azalea Kernel - Main entry point.

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

#include "devices/generic/gen_terminal.h"
#include "devices/generic/gen_keyboard.h"
#include "system_tree/fs/fat/fat_fs.h"
#include "system_tree/fs/pipe/pipe_fs.h"
#include "system_tree/fs/mem/mem_fs.h"
#include "system_tree/fs/dev/dev_fs.h"

#include "entry/multiboot.h"

#include <memory>
#include <stdio.h>

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

/// @cond
extern "C" int main(unsigned int magic_number, multiboot_hdr *mb_header);
/// @endcond

void kernel_start() throw ();
void setup_task_parameters(task_process *startup_proc);

// Temporary procedures and storage while the kernel is being developed. Eventually, the full kernel start procedure
// will cause these to become unused.

std::shared_ptr<task_process> *system_process; ///< The process containing idle processes, etc.
std::shared_ptr<task_process> *kernel_start_process; ///< Process running the kernel start procedure.

extern task_process *term_proc;
task_process *term_proc = nullptr; ///< Process running the main terminal (temporary variable)

extern generic_keyboard *keyb_ptr;
extern std::shared_ptr<terms::generic> *term_ptr;

// Assumptions used throughout the kernel
static_assert(sizeof(uint64_t) == sizeof(uintptr_t), "Code throughout assumes pointers are 64-bits long.");

// There are a few places to check before this assert can be removed - ACPI headers for example.
static_assert(sizeof(unsigned long) == 8, "Unsigned long must be 8 bytes");

/// @brief Main kernel entry point.
///
/// This is called by an assembly-language loader that should do as little as possible. On x64, this involves setting
/// up a simple page mapping, since the kernel is linked higher-half but loaded at 1MB, then kicking the task manager
/// in to life.
///
/// @param magic_number This number should be set to MULTIBOOT_CONSTANT to indicate loading by a multiboot compliant
///                     loader.
///
/// @param mb_header Header of the multiboot structure containing data passed by the bootloader.
///
/// @return This function should never return.
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

/// @brief Main kernel start procedure.
///
/// Started when multi tasking has been enabled and continues the kernel start up procedure.
void kernel_start() throw ()
{
  KL_TRC_TRACE(TRC_LVL::FLOW,
               "Entered kernel start - thread: ",
               reinterpret_cast<uint64_t>(task_get_cur_thread()),
               "\n");

  std::shared_ptr<task_process> initial_proc;
  std::shared_ptr<task_process> com_port_proc;
  std::shared_ptr<ISystemTreeLeaf> leaf;
  char proc_ptr_buffer[34];
  const char hello_string[] = "Hello, world!";
  uint64_t br;
  std::shared_ptr<pipe_branch::pipe_read_leaf> pipe_read_leaf;

  acpi_finish_init();

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Code below here is not intended to be part of the permanent kernel start procedure, but will sit here until the //
  // kernel is more well-developed.                                                                                  //
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // Start the device management system.
  std::shared_ptr<dev_root_branch> dev_root = std::make_shared<dev_root_branch>();
  ASSERT(system_tree()->add_child("dev", dev_root) == ERR_CODE::NO_ERROR);
  dev_root->scan_for_devices();

  ASSERT(system_tree()->get_child("root", leaf) == ERR_CODE::NO_ERROR);

  initial_proc = proc_load_elf_file("root\\initprog");
  setup_task_parameters(initial_proc.get());
  //com_port_proc = proc_load_elf_file("root\\initprog");
  //setup_task_parameters(com_port_proc.get());
  ASSERT(initial_proc != nullptr);
  //ASSERT(com_port_proc != nullptr);

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

  ASSERT(keyb_ptr != nullptr);
  ASSERT(term_ptr != nullptr);

  kl_trc_trace(TRC_LVL::FLOW, "About to start creating pipes\n");

  // Start a simple terminal process.
  std::shared_ptr<IReadable> reader;
  std::shared_ptr<IWritable> stdin_writer;

  std::shared_ptr<ISystemTreeBranch> pipes_br = std::make_shared<system_tree_simple_branch>();
  ASSERT(pipes_br != nullptr);
  ASSERT(system_tree() != nullptr);
  ASSERT(system_tree()->add_child("pipes", pipes_br) == ERR_CODE::NO_ERROR);
  std::shared_ptr<pipe_branch> stdout_br = pipe_branch::create();
  ASSERT(pipes_br->add_child("terminal-output", stdout_br) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->get_child("pipes\\terminal-output\\read", leaf) == ERR_CODE::NO_ERROR);
  reader = std::dynamic_pointer_cast<IReadable>(leaf);
  ASSERT(reader != nullptr);


  // Set up an input pipe (which maps to stdin)
  ASSERT(pipes_br->add_child("terminal-input", pipe_branch::create()) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->get_child("pipes\\terminal-input\\write", leaf) == ERR_CODE::NO_ERROR);
  stdin_writer = std::dynamic_pointer_cast<IWritable>(leaf);
  ASSERT(stdin_writer != nullptr);

  (*term_ptr)->stdin_writer = stdin_writer;
  (*term_ptr)->stdout_reader = reader;
  std::shared_ptr<work::message_receiver> term_rcv = (*term_ptr);
  stdout_br->set_msg_receiver(term_rcv);

  // Setup the write end of the terminal pipe. This is a bit dubious, it doesn't do any reference counting...
  ASSERT(system_tree()->get_child("pipes\\terminal-output\\write", leaf) == ERR_CODE::NO_ERROR);
  snprintf(proc_ptr_buffer, 34, "proc\\%p\\stdout", initial_proc.get());

  KL_TRC_TRACE(TRC_LVL::FLOW, "proc: ", (const char *)proc_ptr_buffer, "\n");
  ASSERT(system_tree()->add_child(proc_ptr_buffer, leaf) == ERR_CODE::NO_ERROR);

  snprintf(proc_ptr_buffer, 34, "proc\\%p\\stderr", initial_proc.get());
  ASSERT(system_tree()->add_child(proc_ptr_buffer, leaf) == ERR_CODE::NO_ERROR);


  snprintf(proc_ptr_buffer, 34, "proc\\%p\\stdin", initial_proc.get());
  ASSERT(system_tree()->get_child("pipes\\terminal-input\\read", leaf) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->add_child(proc_ptr_buffer, leaf) == ERR_CODE::NO_ERROR);
  pipe_read_leaf = std::dynamic_pointer_cast<pipe_branch::pipe_read_leaf>(leaf);
  ASSERT(pipe_read_leaf != nullptr);
  pipe_read_leaf->set_block_on_read(true);

  // Do the same for the process connected to the serial port process.
  /*ASSERT(system_tree()->get_child("pipes\\serial-output\\write", leaf) == ERR_CODE::NO_ERROR);
  snprintf(proc_ptr_buffer, 34, "proc\\%p\\stdout", com_port_proc.get());

  KL_TRC_TRACE(TRC_LVL::FLOW, "proc: ", (const char *)proc_ptr_buffer, "\n");
  ASSERT(system_tree()->add_child(proc_ptr_buffer, leaf) == ERR_CODE::NO_ERROR);

  snprintf(proc_ptr_buffer, 34, "proc\\%p\\stderr", com_port_proc.get());
  ASSERT(system_tree()->add_child(proc_ptr_buffer, leaf) == ERR_CODE::NO_ERROR);

  snprintf(proc_ptr_buffer, 34, "proc\\%p\\stdin", com_port_proc.get());
  ASSERT(system_tree()->get_child("pipes\\serial-input\\read", leaf) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->add_child(proc_ptr_buffer, leaf) == ERR_CODE::NO_ERROR);
  pipe_read_leaf = std::dynamic_pointer_cast<pipe_branch::pipe_read_leaf>(leaf);
  ASSERT(pipe_read_leaf != nullptr);
  pipe_read_leaf->set_block_on_read(true);*/

  // Process should be good to go!
  initial_proc->start_process();
  //com_port_proc->start_process();

  // If (when!) the initial process exits, we want the system to shut down. But since we don't really do shutting down
  // at the moment, just crash instead.
  initial_proc->wait_for_signal();

  panic("System has 'shut down'");
}

/// @brief Setup a plausible argc, argv and environ in startup_proc.
///
/// Let's go for:
/// argc = 2
/// argv = "initprog", "testparam"
/// environ = "OSTYPE=azalea"
///
/// @param startup_proc The process to be treated like 'init' in Linux.
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
