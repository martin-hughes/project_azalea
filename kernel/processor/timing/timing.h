#ifndef __TIMING_H
#define __TIMING_H

#include <stdint.h>

struct time_timer_info
{

};

enum TIMER_MODES
{
  TIMER_PERIODIC,
  TIMER_ONE_OFF,
};

typedef void (*timer_callback)(void *);

void time_gen_init();

void time_sleep_process(uint64_t wait_in_ns);
void time_stall_process(uint64_t wait_in_ns);

uint64_t time_get_system_timer_count(bool output_in_ns = false);
uint64_t time_get_system_timer_offset(uint64_t wait_in_ns);

const unsigned int time_task_mgr_int_period_ns = 100000;

#endif
