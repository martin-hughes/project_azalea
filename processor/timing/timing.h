#ifndef __TIMING_H
#define __TIMING_H

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

void time_sleep_process(unsigned long wait_in_ns);
void time_stall_process(unsigned long wait_in_ns);

const unsigned int time_task_mgr_int_period_ns = 100000;

#endif
