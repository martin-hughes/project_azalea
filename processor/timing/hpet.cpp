// Interface to the HPET.

#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor/timing/timing.h"
#include "processor/timing/hpet.h"
extern "C"
{
#include "acpi/acpica/source/include/acpi.h"
}

hpet_hardware_cfg_block *hpet_config = (hpet_hardware_cfg_block *)NULL;

void time_hpet_set_flag(unsigned long &hpet_reg, const unsigned long flag);
void time_hpet_clear_flag(unsigned long &hpet_reg, const unsigned long flag);
bool time_hpet_get_flag(unsigned long &hpet_reg, const unsigned long flag);
unsigned long time_hpet_compute_wait(unsigned long wait_in_ns);

// Use ACPI to determine whether a HPET exists in this system.
bool time_hpet_exists()
{
  KL_TRC_ENTRY;

  acpi_table_hpet *hpet_table;
  ACPI_STATUS retval;
  bool hpet_exists;
  char table_name[] = "HPET";

  retval = AcpiGetTable(table_name, 0, (ACPI_TABLE_HEADER **)&hpet_table);
  hpet_exists = (retval == AE_OK);

  KL_TRC_EXIT;
  return hpet_exists;
}

// Initialize the HPET according to our requirements - the first channel for the multi-tasking interrupt, the second
// for internal use. It is assumed that time_hpet_exists() would return true when this function is called, otherwise it
// may panic.
void time_hpet_init()
{
  KL_TRC_ENTRY;

  acpi_table_hpet *hpet_table;
  ACPI_STATUS retval;
  char table_name[] = "HPET";
  unsigned long phys_addr;
  unsigned long offset;
  unsigned long virt_addr;
  unsigned long cap_flags;

  retval = AcpiGetTable(table_name, 0, (ACPI_TABLE_HEADER **)&hpet_table);
  ASSERT(retval == AE_OK);
  ASSERT(hpet_table->Address.Address != (unsigned long)NULL);

  // Map the HPET configuration into the kernel's address space.
  phys_addr = hpet_table->Address.Address;
  offset = phys_addr % MEM_PAGE_SIZE;
  // Make sure the HPET config block fits within 1 page.
  ASSERT((offset + sizeof(hpet_hardware_cfg_block)) < MEM_PAGE_SIZE);
  phys_addr -= offset;

  virt_addr = (unsigned long)mem_allocate_virtual_range(1);
  mem_map_range((void *)phys_addr, (void *)virt_addr, 1);
  virt_addr += offset;

  // Perform some basic checks to make sure it is usable.
  hpet_config = (hpet_hardware_cfg_block *)virt_addr;
  cap_flags = hpet_config->gen_cap_and_id;
  ASSERT(HPET_REVISION(cap_flags) != 0);
  ASSERT(HPET_NUM_TIMERS(cap_flags) >= 2);
  ASSERT(HPET_PERIOD(cap_flags) <= max_period_fs);
  ASSERT(time_hpet_get_flag(cap_flags, hpet_hw_leg_rte_cap));

  KL_TRC_TRACE((TRC_LVL_FLOW, "HPET general information:\n"));
  KL_TRC_DATA("Revision", HPET_REVISION(cap_flags));
  KL_TRC_DATA("Number of timers", HPET_NUM_TIMERS(cap_flags));
  KL_TRC_DATA("Period in fs", HPET_PERIOD(cap_flags));

  // Stop the HPET while we configure it.
  time_hpet_clear_flag(hpet_config->gen_config, hpet_cfg_glbl_enable);

  // Keep legacy routing mode enabled, so that we can use different IRQs for timers 0 and 1.
  // (On QEMU at least, it will only route through one interrupt on the IOAPIC for all timers when not in legacy
  // replacement mode).
  time_hpet_set_flag(hpet_config->gen_config, hpet_cfg_leg_rte_map);

  // Configure timer 0 as a 100us periodic timer that calls IRQ 0. IRQ 0 is configured by task_install_task_switcher()
  KL_TRC_DATA("Timer 0 config field before", hpet_config->timer_cfg[0].cfg_and_caps);
  KL_TRC_DATA("Timer 1 config field before", hpet_config->timer_cfg[1].cfg_and_caps);
  ASSERT(time_hpet_get_flag(hpet_config->timer_cfg[0].cfg_and_caps, hpet_tmr_periodic_capable));
  ASSERT(time_hpet_get_flag(hpet_config->timer_cfg[0].cfg_and_caps, hpet_tmr_64_bit_cap));
  //ASSERT((HPET_TMR_INT_RTE_CAP(hpet_config->timer_cfg[0].cfg_and_caps) & 1) != 0);

  //HPET_TMR_SET_INT_RTE(hpet_config->timer_cfg[0].cfg_and_caps, 0);
  time_hpet_clear_flag(hpet_config->timer_cfg[0].cfg_and_caps, hpet_tmr_force_32_bit);
  time_hpet_set_flag(hpet_config->timer_cfg[0].cfg_and_caps, hpet_tmr_periodic);
  time_hpet_clear_flag(hpet_config->timer_cfg[0].cfg_and_caps, hpet_tmr_level_trig_int);

  // Set the period, and reset the main HPET counter to zero, so that it works properly!
  time_hpet_set_flag(hpet_config->timer_cfg[0].cfg_and_caps, hpet_tmr_write_val);
  hpet_config->timer_cfg[0].comparator_val = time_hpet_compute_wait(time_task_mgr_int_period_ns);
  hpet_config->main_counter_val = 0;

  time_hpet_set_flag(hpet_config->timer_cfg[0].cfg_and_caps, hpet_tmr_enable);

  KL_TRC_DATA("Timer 0 config field after", hpet_config->timer_cfg[0].cfg_and_caps);

  // Configure timer 1 as a stopped 1-shot timer that calls IRQ 8.
  ASSERT(time_hpet_get_flag(hpet_config->timer_cfg[1].cfg_and_caps, hpet_tmr_64_bit_cap));
  //ASSERT((HPET_TMR_INT_RTE_CAP(hpet_config->timer_cfg[1].cfg_and_caps) & 0x100) != 0);

  //HPET_TMR_SET_INT_RTE(hpet_config->timer_cfg[1].cfg_and_caps, 8);
  time_hpet_clear_flag(hpet_config->timer_cfg[1].cfg_and_caps, hpet_tmr_force_32_bit);
  time_hpet_clear_flag(hpet_config->timer_cfg[1].cfg_and_caps, hpet_tmr_periodic);
  time_hpet_clear_flag(hpet_config->timer_cfg[1].cfg_and_caps, hpet_tmr_level_trig_int);
  time_hpet_clear_flag(hpet_config->timer_cfg[1].cfg_and_caps, hpet_tmr_enable);

  KL_TRC_DATA("Timer 1 config field after", hpet_config->timer_cfg[1].cfg_and_caps);

  // Resume the HPET
  time_hpet_set_flag(hpet_config->gen_config, hpet_cfg_glbl_enable);

  KL_TRC_EXIT;
}

void time_hpet_set_flag(unsigned long &hpet_reg, const unsigned long flag)
{
  KL_TRC_ENTRY;
  KL_TRC_DATA("Flag being set", flag);

  unsigned long scratch;

  scratch = hpet_reg;
  scratch = scratch | flag;
  hpet_reg = scratch;

  KL_TRC_EXIT;
}

void time_hpet_clear_flag(unsigned long &hpet_reg, const unsigned long flag)
{
  KL_TRC_ENTRY;
  KL_TRC_DATA("Flag being cleared", flag);

  unsigned long scratch;

  scratch = hpet_reg;
  scratch = scratch & ~flag;
  hpet_reg = scratch;

  KL_TRC_EXIT;
}

bool time_hpet_get_flag(unsigned long &hpet_reg, const unsigned long flag)
{
  KL_TRC_ENTRY;

  KL_TRC_DATA("Flag being checked", flag);
  KL_TRC_DATA("Register value    ", hpet_reg);
  bool result = ((hpet_reg & flag) != 0);
  KL_TRC_DATA("Result", (unsigned long)result);

  KL_TRC_EXIT;

  return result;
}

// Compute the value to be written to a HPET timer for the specified wait.
// Makes two assumptions:
// -1: That the wait period is small enough not to overflow the computed result (otherwise the return value is
//     incorrect)
// -2: That the timer's counter starts at zero (the caller can simply add the current value if desired)
unsigned long time_hpet_compute_wait(unsigned long wait_in_ns)
{
  KL_TRC_ENTRY;

  unsigned long wait_in_fs;
  unsigned long result;

  KL_TRC_DATA("Requested period (ns)", wait_in_ns);

  wait_in_fs = wait_in_ns * 1000000;
  result = wait_in_fs / HPET_PERIOD(hpet_config->gen_cap_and_id);

  KL_TRC_DATA("Number of cycles required", result);

  KL_TRC_EXIT;

  return result;
}

void time_hpet_stall(unsigned long wait_in_ns)
{
  KL_TRC_ENTRY;

  unsigned long wait_in_cycles = time_hpet_compute_wait(wait_in_ns);
  unsigned long cur_count;
  unsigned long end_count;

  cur_count = hpet_config->main_counter_val;
  end_count = cur_count + wait_in_cycles;

  KL_TRC_DATA("Wait in ns", wait_in_ns);
  KL_TRC_DATA("Wait in cycles", wait_in_cycles);
  KL_TRC_DATA("Current cycle count", cur_count);
  KL_TRC_DATA("End cycle count", end_count);

  while(end_count > cur_count)
  {
    cur_count = hpet_config->main_counter_val;
  }

  KL_TRC_DATA("Actual end count", cur_count);

  KL_TRC_EXIT;
}
