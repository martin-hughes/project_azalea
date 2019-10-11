#ifndef _PROC_APIC_H
#define _PROC_APIC_H

#include <stdint.h>

#include "pic.h"

// Interface to control regular APICs (not X2 APICs)
void proc_x64_configure_sys_apic_mode(uint32_t num_procs);
void proc_x64_configure_local_apic_mode();
void proc_x64_configure_local_apic();
unsigned char proc_x64_apic_get_local_id();

extern "C" void asm_proc_apic_spurious_interrupt();
extern "C" void proc_x64_apic_irq_ack();

void proc_apic_send_ipi(const uint32_t apic_dest,
                        const PROC_IPI_SHORT_TARGET shorthand,
                        const PROC_IPI_INTERRUPT interrupt,
                        const unsigned char vector,
                        const bool wait_for_delivery);

/// @brief The memory-mapped registers of a Local APIC device.
///
/// Details are contained in the Intel System Programmer's guide so are not repeated.
struct apic_registers
{
  /// @cond
  uint64_t reserved_1[4] __attribute__ ((aligned (16)));

  uint32_t local_apic_id __attribute__ ((aligned (16))); // Offset 0x20: Local APIC ID Register (RO).

  uint32_t local_apic_version __attribute__ ((aligned (16))); // Offset 0x30: Local APIC version register (RO).

  // Padding
  uint32_t reserved_2[16] __attribute__ ((aligned (16)));

  uint32_t task_priority __attribute__ ((aligned (16)));  // Offset 0x80: Task Priority Register (RW)

  uint32_t arbitration_priority __attribute__ ((aligned (16))); // Offset 0x90: Arbitration Priority Register (APR) (RO)

  uint32_t processor_priority __attribute__ ((aligned (16))); // Offset 0xA0: Processor Priority Register (PPR) (RO)

  uint32_t end_of_interrupt __attribute__ ((aligned (16))); // Offset 0xB0: End of Interrupt register (WO)

  uint32_t remote_read __attribute__ ((aligned (16))); // Offset 0xC0: Remote read register (RO) - There's no obvious info for this in the SPG.

  uint32_t logical_destination __attribute__ ((aligned (16))); // Offset 0xD0: Logical Destination (RW)

  uint32_t destination_format __attribute__ ((aligned (16))); // Offset E0: Destination Format (RW)

  uint32_t spurious_interrupt_vector __attribute__ ((aligned (16))); // Offset F0: Spurious Interrupt Vector (RW)

  // In Service Register (RO) - This is a 255 bit register, split in to 128-bit aligned 32-bit chunks. Low order
  // integers first.
  uint32_t in_service_1 __attribute__ ((aligned (16))); // Offset 0x100
  uint32_t in_service_2 __attribute__ ((aligned (16))); // Offset 0x110
  uint32_t in_service_3 __attribute__ ((aligned (16))); // Offset 0x120
  uint32_t in_service_4 __attribute__ ((aligned (16))); // Offset 0x130
  uint32_t in_service_5 __attribute__ ((aligned (16))); // Offset 0x140
  uint32_t in_service_6 __attribute__ ((aligned (16))); // Offset 0x150
  uint32_t in_service_7 __attribute__ ((aligned (16))); // Offset 0x160
  uint32_t in_service_8 __attribute__ ((aligned (16))); // Offset 0x170

  // Trigger Mode Register (TMR) (RO) - Aligned as above.
  uint32_t trigger_mode_1 __attribute__ ((aligned (16))); // Offset 0x180
  uint32_t trigger_mode_2 __attribute__ ((aligned (16))); // Offset 0x190
  uint32_t trigger_mode_3 __attribute__ ((aligned (16))); // Offset 0x1A0
  uint32_t trigger_mode_4 __attribute__ ((aligned (16))); // Offset 0x1B0
  uint32_t trigger_mode_5 __attribute__ ((aligned (16))); // Offset 0x1C0
  uint32_t trigger_mode_6 __attribute__ ((aligned (16))); // Offset 0x1D0
  uint32_t trigger_mode_7 __attribute__ ((aligned (16))); // Offset 0x1E0
  uint32_t trigger_mode_8 __attribute__ ((aligned (16))); // Offset 0x1F0

  // Interrupt Request Register (IRR) (RO) - Also as above
  uint32_t interrupt_request_1 __attribute__ ((aligned (16))); // Offset 0x200
  uint32_t interrupt_request_2 __attribute__ ((aligned (16))); // Offset 0x210
  uint32_t interrupt_request_3 __attribute__ ((aligned (16))); // Offset 0x220
  uint32_t interrupt_request_4 __attribute__ ((aligned (16))); // Offset 0x230
  uint32_t interrupt_request_5 __attribute__ ((aligned (16))); // Offset 0x240
  uint32_t interrupt_request_6 __attribute__ ((aligned (16))); // Offset 0x250
  uint32_t interrupt_request_7 __attribute__ ((aligned (16))); // Offset 0x260
  uint32_t interrupt_request_8 __attribute__ ((aligned (16))); // Offset 0x270

  uint32_t error_status __attribute__ ((aligned (16))); // Offset 0x280: Error status (RO)

  uint32_t reserved_3[24] __attribute__ ((aligned (16)));

  uint32_t lvt_cmci __attribute__ ((aligned (16))); // Offset 0x2F0: CMCI (RW)

  // Offset 0x300: Interrupt command bits 0-31 (RW)
  volatile uint32_t lvt_interrupt_command_1 __attribute__ ((aligned (16)));
  uint32_t lvt_interrupt_command_2 __attribute__ ((aligned (16))); // Offset 0x310: Interrupt command bit 32-63 (RW)

  uint32_t lvt_timer __attribute__ ((aligned (16))); // Offset 0x320: Timer register (RW)

  uint32_t lvt_thermal_sensor __attribute__ ((aligned (16))); // Offset 0x330: Thermal Sensor (RW)

  uint32_t lvt_perf_mon_counters __attribute__ ((aligned (16))); // Offset 0x340: Performance Monitoring Counters (RW)

  uint32_t lvt_lint0 __attribute__ ((aligned (16))); // Offset 0x350: LVT LINT0
  uint32_t lvt_lint1 __attribute__ ((aligned (16))); // Offset 0x360: LVT LINT1

  uint32_t lvt_error __attribute__ ((aligned (16))); // Offset 0x370: LVT Error Status (RW)

  uint32_t initial_count __attribute__ ((aligned (16))); // Offset 0x380: Timer's initial count register (RW)
  uint32_t current_count __attribute__ ((aligned (16))); // Offset 0x390: Timer's current count register (RO)

  uint32_t reserved_4[16] __attribute__ ((aligned (16)));

  uint32_t divide_config __attribute__ ((aligned (16))); // Offset 0x3E0: Timer's divide configuration register (RW)

  uint32_t reserved_5[4] __attribute__ ((aligned (16)));
  /// @endcond
};

#endif
