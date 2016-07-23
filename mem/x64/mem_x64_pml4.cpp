// Manages a list of known PML4 tables. This is used to ensure that while each process has its own complete set of page
// tables (of which the PML4 is root), the "kernel part" of those tables can be kept synchronized.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "mem/mem.h"
#include "mem/x64/mem_internal_x64.h"
#include "processor/processor.h"

bool pml4_system_initialized = false;
klib_list pml4_table_list;
char pml4_copy_buffer[PML4_LENGTH / 2];
unsigned long known_pml4s;

void mem_x64_pml4_init_sys(process_x64_data &task0_data)
{
  KL_TRC_ENTRY;

  ASSERT(!pml4_system_initialized);
  klib_list_initialize(&pml4_table_list);
  klib_list_item_initialize(&task0_data.pml4_list_item);
  task0_data.pml4_list_item.item = (void *)&task0_data;
  klib_list_add_head(&pml4_table_list, &task0_data.pml4_list_item);
  pml4_system_initialized = true;
  known_pml4s = 1;

  KL_TRC_EXIT;
}

void mem_x64_pml4_allocate(process_x64_data &new_proc_data)
{
  KL_TRC_ENTRY;

  unsigned long offset_in_page;
  unsigned long virtual_page_addr;
  unsigned long physical_page_addr;
  unsigned char *new_pte;
  unsigned char *existing_pte;

  ASSERT(pml4_system_initialized);

  klib_list_item_initialize(&new_proc_data.pml4_list_item);
  new_proc_data.pml4_list_item.item = (void *)&new_proc_data;
  klib_list_add_tail(&pml4_table_list, &new_proc_data.pml4_list_item);

  COMPILER_ASSERT(sizeof(char) == 1)

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

  KL_TRC_EXIT;
}

void mem_x64_pml4_deallocate(process_x64_data &proc_data)
{
  panic("mem_x64_pml4_deallocate not implemented");
}

// Synchronize the kernel part of all the PML4 tables, so that no matter which process calls in to the kernel, they see
// the same page mapping within the kernel.
// TODO: Definitely ought to add some locking here! (MT) (LOCK)
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

  kl_memcpy((void *)updated_kernel_section, (void *)pml4_copy_buffer, PML4_LENGTH / 2);

  for(list_item = pml4_table_list.head; list_item != NULL; list_item = list_item->next)
  {
    proc_data = (process_x64_data *)list_item->item;
    pml4_destination = (unsigned char *)(proc_data->pml4_virt_addr + PML4_LENGTH / 2);
    kl_memcpy((void *)pml4_copy_buffer, (void *)pml4_destination, PML4_LENGTH / 2);

    updated_pml4s++;
  }

  ASSERT(updated_pml4s == known_pml4s);

  KL_TRC_EXIT;
}
