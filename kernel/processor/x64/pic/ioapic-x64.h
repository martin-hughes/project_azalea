#ifndef __IOAPIC_X64_H
#define __IOAPIC_X64_H

#include <stdint.h>

void proc_x64_ioapic_load_data();
uint64_t proc_x64_ioapic_get_count();

void proc_x64_ioapic_remap_interrupts(uint32_t ioapic_num, uint8_t base_int, uint8_t apic_id);

#endif
