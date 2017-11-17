#ifndef _PROC_APIC_H
#define _PROC_APIC_H

#include "pic.h"

// Interface to control regular APICs (not X2 APICs)
void proc_x64_configure_sys_apic_mode(unsigned int num_procs);
void proc_x64_configure_local_apic_mode();
void proc_x64_configure_local_apic();
unsigned char proc_x64_apic_get_local_id();

extern "C" void asm_proc_apic_spurious_interrupt();
extern "C" void proc_x64_apic_irq_ack();

void proc_apic_send_ipi(const unsigned int apic_dest,
                        const PROC_IPI_SHORT_TARGET shorthand,
                        const PROC_IPI_INTERRUPT interrupt,
                        const unsigned char vector,
                        const bool wait_for_delivery);

struct apic_registers
{
  unsigned long reserved_1[4] __attribute__ ((aligned (16)));

  unsigned int local_apic_id __attribute__ ((aligned (16))); // Offset 0x20: Local APIC ID Register (RO).

  unsigned int local_apic_version __attribute__ ((aligned (16))); // Offset 0x30: Local APIC version register (RO).

  // Padding
  unsigned int reserved_2[16] __attribute__ ((aligned (16)));

  unsigned int task_priority __attribute__ ((aligned (16)));  // Offset 0x80: Task Priority Register (RW)

  unsigned int arbitration_priority __attribute__ ((aligned (16))); // Offset 0x90: Arbitration Priority Register (APR) (RO)

  unsigned int processor_priority __attribute__ ((aligned (16))); // Offset 0xA0: Processor Priority Register (PPR) (RO)

  unsigned int end_of_interrupt __attribute__ ((aligned (16))); // Offset 0xB0: End of Interrupt register (WO)

  unsigned int remote_read __attribute__ ((aligned (16))); // Offset 0xC0: Remote read register (RO) - There's no obvious info for this in the SPG.

  unsigned int logical_destination __attribute__ ((aligned (16))); // Offset 0xD0: Logical Destination (RW)

  unsigned int destination_format __attribute__ ((aligned (16))); // Offset E0: Destination Format (RW)

  unsigned int spurious_interrupt_vector __attribute__ ((aligned (16))); // Offset F0: Spurious Interrupt Vector (RW)

  // In Service Register (RO) - This is a 255 bit register, split in to 128-bit aligned 32-bit chunks. Low order
  // integers first.
  unsigned int in_service_1 __attribute__ ((aligned (16))); // Offset 0x100
  unsigned int in_service_2 __attribute__ ((aligned (16))); // Offset 0x110
  unsigned int in_service_3 __attribute__ ((aligned (16))); // Offset 0x120
  unsigned int in_service_4 __attribute__ ((aligned (16))); // Offset 0x130
  unsigned int in_service_5 __attribute__ ((aligned (16))); // Offset 0x140
  unsigned int in_service_6 __attribute__ ((aligned (16))); // Offset 0x150
  unsigned int in_service_7 __attribute__ ((aligned (16))); // Offset 0x160
  unsigned int in_service_8 __attribute__ ((aligned (16))); // Offset 0x170

  // Trigger Mode Register (TMR) (RO) - Aligned as above.
  unsigned int trigger_mode_1 __attribute__ ((aligned (16))); // Offset 0x180
  unsigned int trigger_mode_2 __attribute__ ((aligned (16))); // Offset 0x190
  unsigned int trigger_mode_3 __attribute__ ((aligned (16))); // Offset 0x1A0
  unsigned int trigger_mode_4 __attribute__ ((aligned (16))); // Offset 0x1B0
  unsigned int trigger_mode_5 __attribute__ ((aligned (16))); // Offset 0x1C0
  unsigned int trigger_mode_6 __attribute__ ((aligned (16))); // Offset 0x1D0
  unsigned int trigger_mode_7 __attribute__ ((aligned (16))); // Offset 0x1E0
  unsigned int trigger_mode_8 __attribute__ ((aligned (16))); // Offset 0x1F0

  // Interrupt Request Register (IRR) (RO) - Also as above
  unsigned int interrupt_request_1 __attribute__ ((aligned (16))); // Offset 0x200
  unsigned int interrupt_request_2 __attribute__ ((aligned (16))); // Offset 0x210
  unsigned int interrupt_request_3 __attribute__ ((aligned (16))); // Offset 0x220
  unsigned int interrupt_request_4 __attribute__ ((aligned (16))); // Offset 0x230
  unsigned int interrupt_request_5 __attribute__ ((aligned (16))); // Offset 0x240
  unsigned int interrupt_request_6 __attribute__ ((aligned (16))); // Offset 0x250
  unsigned int interrupt_request_7 __attribute__ ((aligned (16))); // Offset 0x260
  unsigned int interrupt_request_8 __attribute__ ((aligned (16))); // Offset 0x270

  unsigned int error_status __attribute__ ((aligned (16))); // Offset 0x280: Error status (RO)

  unsigned int reserved_3[24] __attribute__ ((aligned (16)));

  unsigned int lvt_cmci __attribute__ ((aligned (16))); // Offset 0x2F0: CMCI (RW)

  // Offset 0x300: Interrupt command bits 0-31 (RW)
  volatile unsigned int lvt_interrupt_command_1 __attribute__ ((aligned (16)));
  unsigned int lvt_interrupt_command_2 __attribute__ ((aligned (16))); // Offset 0x310: Interrupt command bit 32-63 (RW)

  unsigned int lvt_timer __attribute__ ((aligned (16))); // Offset 0x320: Timer register (RW)

  unsigned int lvt_thermal_sensor __attribute__ ((aligned (16))); // Offset 0x330: Thermal Sensor (RW)

  unsigned int lvt_perf_mon_counters __attribute__ ((aligned (16))); // Offset 0x340: Performance Monitoring Counters (RW)

  unsigned int lvt_lint0 __attribute__ ((aligned (16))); // Offset 0x350: LVT LINT0
  unsigned int lvt_lint1 __attribute__ ((aligned (16))); // Offset 0x360: LVT LINT1

  unsigned int lvt_error __attribute__ ((aligned (16))); // Offset 0x370: LVT Error Status (RW)

  unsigned int initial_count __attribute__ ((aligned (16))); // Offset 0x380: Timer's initial count register (RW)
  unsigned int current_count __attribute__ ((aligned (16))); // Offset 0x390: Timer's current count register (RO)

  unsigned int reserved_4[16] __attribute__ ((aligned (16)));

  unsigned int divide_config __attribute__ ((aligned (16))); // Offset 0x3E0: Timer's divide configuration register (RW)

  unsigned int reserved_5[4] __attribute__ ((aligned (16)));
};

#endif
