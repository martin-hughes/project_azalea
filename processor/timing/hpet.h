#ifndef __HPET_H
#define __HPET_H

bool time_hpet_exists();
void time_hpet_init();

void time_hpet_stall(unsigned long wait_in_ns);

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
  unsigned long cfg_and_caps;
  unsigned long comparator_val;
  unsigned long interrupt_route;
  unsigned long reserved;
};

struct hpet_hardware_cfg_block
{
  unsigned long gen_cap_and_id;
  unsigned long reserved_1;
  unsigned long gen_config;
  unsigned long reserved_2;
  unsigned long gen_int_status;
  unsigned long reserved_3[25];
  unsigned long main_counter_val;
  unsigned long reserved_4;
  hpet_timer_cfg timer_cfg[32];
};
#pragma pack(pop)

// Fields in hpet_hardware_cfg_block.gen_cap_and_id
#define HPET_PERIOD(x) ((x) >> 32)
#define HPET_NUM_TIMERS(x)  ((((x) >> 8) & (0x1F)) + 1)
#define HPET_REVISION(x) ((x) & 0xFF)

// Flags within hpet_hardware_cfg_block.gen_cap_and_id
const unsigned long hpet_hw_leg_rte_cap = (1 << 15);

// Flags within hpet_hardware_cfg_block.gen_config
const unsigned long hpet_cfg_leg_rte_map = 2;
const unsigned long hpet_cfg_glbl_enable = 1;

// Flags within hpet_timer_cfg.cfg_and_caps
const unsigned long hpet_tmr_level_trig_int = 2;
const unsigned long hpet_tmr_enable = 4;
const unsigned long hpet_tmr_periodic = 8;
const unsigned long hpet_tmr_periodic_capable = 16;
const unsigned long hpet_tmr_64_bit_cap = 32;
const unsigned long hpet_tmr_write_val = 64;
const unsigned long hpet_tmr_force_32_bit = 256;
const unsigned long hpet_tmr_fsb_int_enable = (1 << 14);
const unsigned long hpet_tmr_fsb_int_cap = (1 << 15);

// Fields within hpet_timer_cfg.cfg_and_caps
#define HPET_TMR_INT_RTE_CAP(x) ((x) >> 32)
#define HPET_TMR_GET_INT_RTE(x) (((x) >> 9) & 0x0F)
#define HPET_TMR_SET_INT_RTE(x, rte) \
      { \
        unsigned long DEF_scratch = (x); \
        DEF_scratch = DEF_scratch & ~((0x0F) << 9); \
        DEF_scratch = DEF_scratch | ((rte) << 9); \
        x = DEF_scratch; \
      }

const unsigned long max_period_fs = 0x05F5E100;



#endif
