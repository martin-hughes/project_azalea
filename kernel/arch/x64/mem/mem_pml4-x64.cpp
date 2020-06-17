/// @file
/// @brief Manages all known PML4 tables in the system.
///
/// The PML4 table is the root of the page table tree. Each process in the system has its own set of page tables, and
/// hence, its own PML4 table. The second half of the PML4 represents entries that map the kernel. Editing one PML4 is
/// normally independent of all the others, but this means that the kernel could edit one PML4 and find itself unable
/// to resolve some important variable after the processor selects a new set of page tables.
///
/// As such, this code keeps the kernel specific part of every known PML4 in synch with the others.

//#define ENABLE_TRACING

#include "mem.h"
#include "mem-x64.h"
#include "processor.h"

static bool pml4_system_initialized = false; ///< Is the PML4 tracking system initalised?
static klib_list<process_x64_data *> pml4_table_list; ///< A list of known PML4 tables.
/// Space to use when copying PML4 tables. Only half of the full PML4 size is needed, because we only synchronise the
/// kernel parts.
static int8_t pml4_copy_buffer[PML4_LENGTH / 2];
static uint64_t known_pml4s; ///< How many PML4s are known in the system?
static ipc::raw_spinlock pml4_copylock; ///< Lock to ensure PML4s are copied in sequence.

static_assert(sizeof(char) == 1, "Only single byte characters supported right now");

/// @brief Initialise the PML4 management system.
///
/// **Must only be called once!**
///
/// @param task0_data The x64-specific part of the process information for task 0 (which is the task that is nominally
///                   running before the kernel starts tasking properly)
void mem_x64_pml4_init_sys(process_x64_data &task0_data)
{
  KL_TRC_ENTRY;

  ASSERT(!pml4_system_initialized);
  klib_list_initialize(&pml4_table_list);
  klib_list_item_initialize(&task0_data.pml4_list_item);
  task0_data.pml4_list_item.item = &task0_data;
  klib_list_add_head(&pml4_table_list, &task0_data.pml4_list_item);
  known_pml4s = 1;
  ipc_raw_spinlock_init(pml4_copylock);
  pml4_system_initialized = true;

  KL_TRC_EXIT;
}

/// @brief Allocate and start tracking the page tables for a new process
///
/// @param new_proc_data The x64-specific part of the process information for the newly-created process.
void mem_x64_pml4_allocate(process_x64_data &new_proc_data)
{
  KL_TRC_ENTRY;

  uint64_t offset_in_page;
  uint64_t virtual_page_addr;
  uint64_t physical_page_addr;
  uint8_t *new_pte;
  uint8_t *existing_pte;

  ASSERT(pml4_system_initialized);

  ipc_raw_spinlock_lock(pml4_copylock);

  klib_list_item_initialize(&new_proc_data.pml4_list_item);
  new_proc_data.pml4_list_item.item = &new_proc_data;
  klib_list_add_tail(&pml4_table_list, &new_proc_data.pml4_list_item);

  // Simply allocate a 4096 byte table. KLIB will make sure this is in the kernel's address space automatically.
  // The Virtual address is easy.
  new_pte = new uint8_t[PML4_LENGTH];
  KL_TRC_TRACE(TRC_LVL::EXTRA, "New PML4 Virtual Address", new_pte, "\n");
  ASSERT(((uint64_t)new_pte) % PML4_LENGTH == 0);
  memset((void *)new_pte, 0, PML4_LENGTH);

  // Copy a kernel PML4 into this one. All the others should be the same, just pick the first one off of the list.
  existing_pte = (uint8_t *)((process_x64_data *)pml4_table_list.head->item)->pml4_virt_addr;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Copying PML4 from", existing_pte, "\n");
  memcpy(new_pte + (PML4_LENGTH / 2), existing_pte + (PML4_LENGTH / 2), PML4_LENGTH / 2);

  // Compute the physical address. Start off by figuring out which virtual page it's in, which allows the mapping to
  // physical pages to be computed. The physical address of the PML4 is at the same offset as in the virtual page.
  offset_in_page = ((uint64_t)new_pte) % MEM_PAGE_SIZE;
  virtual_page_addr = ((uint64_t)new_pte) - offset_in_page;
  physical_page_addr = (uint64_t)mem_get_phys_addr((void *)virtual_page_addr);

  new_proc_data.pml4_virt_addr = (uint64_t)new_pte;
  new_proc_data.pml4_phys_addr = physical_page_addr + offset_in_page;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "New PML4 Physical address", new_proc_data.pml4_phys_addr, "\n");

  known_pml4s++;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Number of known PML4 tables", known_pml4s, "\n");

  ipc_raw_spinlock_unlock(pml4_copylock);

  KL_TRC_EXIT;
}

/// @brief Stop tracking and deallocate a PML4 table for a process that is terminating.
///
/// @param proc_data The x64-specific part of the process data for the terminating process.
void mem_x64_pml4_deallocate(process_x64_data &proc_data)
{
  KL_TRC_ENTRY;

  ASSERT(pml4_system_initialized);

  ipc_raw_spinlock_lock(pml4_copylock);
  klib_list_remove(&proc_data.pml4_list_item);
  delete[] reinterpret_cast<uint8_t *>(proc_data.pml4_virt_addr);
  ipc_raw_spinlock_unlock(pml4_copylock);

  KL_TRC_EXIT;
}

/// @brief Synchronise the kernel part of all the PML4 tables
///
/// This means that no matter which process has its page tables loaded by the processor, the kernel always sees the
/// same set of mappings for kernel space.
///
/// **It is the caller's responsibility to make sure that no other PML4 changes before this function returns.**
/// Otherwise some PML4s might have the new data and others not, or the newer changes might be obliterated entirely.
/// This is currently achieved by a lock in mem_map_virtual_page, which is the only function that directly edits PML4
/// tables.
///
/// @param updated_pml4_table The PML4 that has changed. All others will be made to be the same as this.
void mem_x64_pml4_synchronize(void *updated_pml4_table)
{
  KL_TRC_ENTRY;

  uint8_t *updated_kernel_section;
  klib_list_item<process_x64_data *> *list_item;
  process_x64_data *proc_data;
  uint8_t *pml4_destination;
  uint64_t updated_pml4s = 0;

  updated_kernel_section = (uint8_t*)updated_pml4_table;
  updated_kernel_section += PML4_LENGTH / 2;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "About to synchronize top part of PML4, starting at address",
               (uint64_t)updated_kernel_section, "\n");

  ipc_raw_spinlock_lock(pml4_copylock);
  memcpy((void *)pml4_copy_buffer, (void *)updated_kernel_section, PML4_LENGTH / 2);

  for(list_item = pml4_table_list.head; list_item != nullptr; list_item = list_item->next)
  {
    proc_data = (process_x64_data *)list_item->item;
    pml4_destination = (uint8_t *)(proc_data->pml4_virt_addr + PML4_LENGTH / 2);
    KL_TRC_TRACE(TRC_LVL::FLOW, "Copying from ", pml4_copy_buffer, " to ", pml4_destination, "\n");
    memcpy((void *)pml4_destination, (void *)pml4_copy_buffer, PML4_LENGTH / 2);

    updated_pml4s++;
  }
  ipc_raw_spinlock_unlock(pml4_copylock);

  ASSERT(updated_pml4s == known_pml4s);

  KL_TRC_EXIT;
}
