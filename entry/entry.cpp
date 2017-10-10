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
#include "system_tree/process/process.h"

#include "devices/block/ata/ata.h"
#include "devices/block/proxy/block_proxy.h"
#include "system_tree/fs/fat/fat_fs.h"
#include "system_tree/fs/pipe/pipe_fs.h"

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

extern "C" int main();
void kernel_start();
void simple_terminal();

// Temporary procedures and storage while the kernel is being developed. Eventually, the full kernel start procedure
// will cause these to become unused.
void setup_initial_fs();
// Some variables to support loading a filesystem.
generic_ata_device *first_hdd;
fat_filesystem *first_fs;

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
  task_process *initial_proc;

  // kernel_start() runs on the BSP. Bring up the APs so they are ready to take on any threads created below.
  proc_mp_start_aps();

  // Bring the ACPI system up to full readiness.
  status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
  ASSERT(status == AE_OK);

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Code below here is not intended to be part of the permanent kernel start procedure, but will sit here until the //
  // kernel is more well-developed.                                                                                  //
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  setup_initial_fs();
  ASSERT(first_fs != nullptr);
  ASSERT(system_tree()->add_branch("root", first_fs) == ERR_CODE::NO_ERROR);

  initial_proc = proc_load_elf_file("root\\testprog");
  ASSERT(initial_proc != nullptr);

  // Start a simple terminal process.
  task_process *term = task_create_new_process(simple_terminal, true);
  task_start_process(term);

  // Process should be good to go!
  task_start_process(initial_proc);

  while (1)
  {
    //Spin forever.
  }
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
  KL_TRC_TRACE(TRC_LVL::EXTRA, (unsigned long)sector_buffer[510], " ", (unsigned long)sector_buffer[511], "\n");
  ASSERT((sector_buffer[510] == 0x55) && (sector_buffer[511] == 0xAA));

  unsigned int start_sector;
  unsigned int sector_count;

  // Parse the MBR to find the first partition.
  kl_memcpy(sector_buffer.get() + 454, &start_sector, 4);
  kl_memcpy(sector_buffer.get() + 458, &sector_count, 4);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "First partition: ", (unsigned long)start_sector, " -> +", (unsigned long)sector_count, "\n");
  block_proxy_device *pd = new block_proxy_device(first_hdd, start_sector, sector_count);

  // Initialise the filesystem based on that information
  first_fs = new fat_filesystem(pd);

  KL_TRC_EXIT;
}

// A simple text based terminal outputting on the main display (output only right now.)
void simple_terminal()
{
  KL_TRC_ENTRY;

  ISystemTreeLeaf *leaf;
  IReadable *reader;
  const unsigned long buffer_size = 10;
  unsigned char buffer[buffer_size];
  unsigned long bytes_read;
  unsigned char *display_ptr;

  const unsigned int width = 80;
  const unsigned int height = 25;
  const unsigned int bytes_per_char = 2;

  unsigned int cur_offset = 0;

  // Set up the input pipe
  ISystemTreeBranch *pipes_br = new system_tree_simple_branch();
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

  KL_TRC_TRACE(TRC_LVL::FLOW, "Beginning terminal\n");
  while(1)
  {
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
