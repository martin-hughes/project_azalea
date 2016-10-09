#ifndef _MEM_INTERNAL_H
#define _MEM_INTERNAL_H

void mem_init_gen_phys_sys();
void mem_gen_phys_pages_bitmap(unsigned long *bitmap_loc,
                               unsigned long max_num_pages);


void mem_set_bitmap_page_bit(unsigned long page_addr, const bool ignore_checks);
void mem_clear_bitmap_page_bit(unsigned long page_addr);
bool mem_is_bitmap_page_bit_set(unsigned long page_addr);

void mem_map_virtual_page(unsigned long virt_addr,
                          unsigned long phys_addr,
                          task_process *context = nullptr,
                          MEM_CACHE_MODES cache_mode = MEM_WRITE_BACK);
void mem_unmap_virtual_page(unsigned long virt_addr);

// Allow the whole memory manager to access the data about task 0 (the kernel),
// to make it easier to feed it into other parts of the system later.
extern mem_process_info task0_entry;

#endif
