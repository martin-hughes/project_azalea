#ifndef __IOAPIC_X64_H
#define __IOAPIC_X64_H

void proc_x64_ioapic_load_data();
unsigned long proc_x64_ioapic_get_count();

void proc_x64_ioapic_remap_interrupts(unsigned int ioapic_num, unsigned char base_int, unsigned char apic_id);

#endif
