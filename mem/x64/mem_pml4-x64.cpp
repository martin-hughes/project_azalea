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

#include "klib/klib.h"
#include "mem/mem.h"
#include "mem/x64/mem-x64-int.h"
#include "processor/processor.h"

static bool pml4_system_initialized = false;
static klib_list pml4_table_list;
static char pml4_copy_buffer[PML4_LENGTH / 2];
static unsigned long known_pml4s;
static kernel_spinlock pml4_copylock;

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
  task0_data.pml4_list_item.item = (void *)&task0_data;
  klib_list_add_head(&pml4_table_list, &task0_data.pml4_list_item);
  known_pml4s = 1;
  klib_synch_spinlock_init(pml4_copylock);
  pml4_system_initialized = true;

  KL_TRC_EXIT;
}

/// @brief Allocate and start tracking the page tables for a new process
///
/// @param new_proc_data The x64-specific part of the process information for the newly-created process.
void mem_x64_pml4_allocate(process_x64_data &new_proc_data)
{
  KL_TRC_ENTRY;

  unsigned long offset_in_page;
  unsigned long virtual_page_addr;
  unsigned long physical_page_addr;
  unsigned char *new_pte;
  unsigned char *existing_pte;

  ASSERT(pml4_system_initialized);

  klib_synch_spinlock_lock(pml4_copylock);

  klib_list_item_initialize(&new_proc_data.pml4_list_item);
  new_proc_data.pml4_list_item.item = (void *)&new_proc_data;
  klib_list_add_tail(&pml4_table_list, &new_proc_data.pml4_list_item);

  // Simply allocate a 4096 byte table. KLIB will make sure this is in the kernel's address space automatically.
  // The Virtual address is easy.
  new_pte = new unsigned char[PML4_LENGTH];
  KL_TRC_DATA("New PML4 Virtual Address", (unsigned long)new_pte);
  ASSERT(((unsigned long)new_pte) % PML4_LENGTH == 0);
  kl_memset((void *)new_pte, 0, PML4_LENGTH);

  // Copy a kernel PML4 into this one. All the others should be the same, just pick the first one off of the list.
  existing_pte = (unsigned char *)((process_x64_data *)pml4_table_list.head->item)->pml4_virt_addr;
  KL_TRC_DATA("Copying PML4 from", (unsigned long)existing_pte);
  kl_memcpy(existing_pte + (PML4_LENGTH / 2), new_pte + (PML4_LENGTH / 2), PML4_LENGTH / 2);

  // Compute the physical address. Start off by figuring out which virtual page it's in, which allows the mapping to
  // physical pages to be computed. The physical address of the PML4 is at the same offset as in the virtual page.
  offset_in_page = ((unsigned long)new_pte) % MEM_PAGE_SIZE;
  virtual_page_addr = ((unsigned long)new_pte) - offset_in_page;
  physical_page_addr = (unsigned long)mem_get_phys_addr((void *)virtual_page_addr);

  new_proc_data.pml4_virt_addr = (unsigned long)new_pte;
  new_proc_data.pml4_phys_addr = physical_page_addr + offset_in_page;
  KL_TRC_DATA("New PML4 Physical address", new_proc_data.pml4_phys_addr);

  known_pml4s++;
  KL_TRC_DATA("Number of known PML4 tables", known_pml4s);

  klib_synch_spinlock_unlock(pml4_copylock);

  KL_TRC_EXIT;
}

/// @brief Stop tracking and deallocate a PML4 table for a process that is terminating.
///
/// @param proc_data The x64-specific part of the process data for the terminating process.
void mem_x64_pml4_deallocate(process_x64_data &proc_data)
{
  panic("mem_x64_pml4_deallocate not implemented");
}

/// @brief Synchronise the kernel part of all the PML4 tables
///
/// This means that no matter which process has its page tables loaded by the processor, the kernel always sees the
/// same set of mappings for kernel space.
///
/// **It is the caller's responsibility to make sure that no other PML4 changes before this function returns.**
/// Otherwise some PML4s might have the new data and others not, or the newer changes might be obliterated entirely.
///
/// @param updated_pml4_table The PML4 that has changed. All others will be made to be the same as this.
void mem_x64_pml4_synchronize(void *updated_pml4_table)
{
  KL_TRC_ENTRY;

  unsigned char *updated_kernel_section;
  klib_list_item *list_item;
  process_x64_data *proc_data;
  unsigned char *pml4_destination;
  unsigned long updated_pml4s = 0;

  updated_kernel_section = (unsigned char*)updated_pml4_table;
  updated_kernel_section += PML4_LENGTH / 2;
  KL_TRC_DATA("About to synchronize top part of PML4, starting at address", (unsigned long)updated_kernel_section);

  klib_synch_spinlock_lock(pml4_copylock);
  kl_memcpy((void *)updated_kernel_section, (void *)pml4_copy_buffer, PML4_LENGTH / 2);

  for(list_item = pml4_table_list.head; list_item != NULL; list_item = list_item->next)
  {
    proc_data = (process_x64_data *)list_item->item;
    pml4_destination = (unsigned char *)(proc_data->pml4_virt_addr + PML4_LENGTH / 2);
    kl_memcpy((void *)pml4_copy_buffer, (void *)pml4_destination, PML4_LENGTH / 2);

    updated_pml4s++;
  }
  klib_synch_spinlock_unlock(pml4_copylock);

  ASSERT(updated_pml4s == known_pml4s);

  KL_TRC_EXIT;
}
