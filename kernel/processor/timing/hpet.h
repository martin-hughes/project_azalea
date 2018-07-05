#ifndef __HPET_H
#define __HPET_H

#include <stdint.h>

bool time_hpet_exists();
void time_hpet_init();

void time_hpet_stall(uint64_t wait_in_ns);
uint64_t time_hpet_cur_value();
uint64_t time_hpet_compute_wait(uint64_t wait_in_ns);

/*
000-007h General Capabilities and ID Register Read Only
008-00Fh Reserved
010-017h General Configuration Register Read-Write
018-01Fh Reserved
020-027h General Interrupt Status Register Read/Write Clear
028-0EFh Reserved
0F0-0F7h Main Counter Value Register Read/Write
0F8-0FFh Reserved
100-107h Timer 0 Configuration and Capability Register Read/Write
108-10Fh Timer 0 Comparator Value Register Read/Write
110-117h Timer 0 FSB Interrupt Route Register Read/Write
118-11Fh Reserved
120-127h Timer 1 Configuration and Capability Register Read/Write
128-12Fh Timer 1 Comparator Value Register Read/Write
130-137h Timer 1 FSB Interrupt Route Register Read/Write
138-13Fh Reserved
140-147h Timer 2 Configuration and Capability Register Read/Write
148-14Fh Timer 2 Comparator Value Register Read/Write
150-157h Timer 2 FSB Interrupt Route Register Read/Write
158-15Fh Reserved
160-3FFh Reserved for Timers 3-31*/

#pragma pack(push, 1)
struct hpet_timer_cfg
{
  uint64_t cfg_and_caps;
  uint64_t comparator_val;
  uint64_t interrupt_route;
  uint64_t reserved;
};

struct hpet_hardware_cfg_block
{
  uint64_t gen_cap_and_id;
  uint64_t reserved_1;
  uint64_t gen_config;
  uint64_t reserved_2;
  uint64_t gen_int_status;
  uint64_t reserved_3[25];
  uint64_t main_counter_val;
  uint64_t reserved_4;
  hpet_timer_cfg timer_cfg[32];
};
#pragma pack(pop)

// Fields in hpet_hardware_cfg_block.gen_cap_and_id
#define HPET_PERIOD(x) ((x) >> 32)
#define HPET_NUM_TIMERS(x)  ((((x) >> 8) & (0x1F)) + 1)
#define HPET_REVISION(x) ((x) & 0xFF)

// Flags within hpet_hardware_cfg_block.gen_cap_and_id
const uint64_t hpet_hw_leg_rte_cap = (1 << 15);

// Flags within hpet_hardware_cfg_block.gen_config
const uint64_t hpet_cfg_leg_rte_map = 2;
const uint64_t hpet_cfg_glbl_enable = 1;

// Flags within hpet_timer_cfg.cfg_and_caps
const uint64_t hpet_tmr_level_trig_int = 2;
const uint64_t hpet_tmr_enable = 4;
const uint64_t hpet_tmr_periodic = 8;
const uint64_t hpet_tmr_periodic_capable = 16;
const uint64_t hpet_tmr_64_bit_cap = 32;
const uint64_t hpet_tmr_write_val = 64;
const uint64_t hpet_tmr_force_32_bit = 256;
const uint64_t hpet_tmr_fsb_int_enable = (1 << 14);
const uint64_t hpet_tmr_fsb_int_cap = (1 << 15);

// Fields within hpet_timer_cfg.cfg_and_caps
#define HPET_TMR_INT_RTE_CAP(x) ((x) >> 32)
#define HPET_TMR_GET_INT_RTE(x) (((x) >> 9) & 0x0F)
#define HPET_TMR_SET_INT_RTE(x, rte) \
      { \
        uint64_t DEF_scratch = (x); \
        DEF_scratch = DEF_scratch & ~((0x0F) << 9); \
        DEF_scratch = DEF_scratch | ((rte) << 9); \
        x = DEF_scratch; \
      }

const uint64_t max_period_fs = 0x05F5E100;



#endif
