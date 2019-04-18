/// @file
/// @brief Declare timing related functionality.

#include <stdint.h>
#include <memory>
#include "user_interfaces/kernel_types.h"

struct time_timer_info
{

};

enum TIMER_MODES
{
  TIMER_PERIODIC,
  TIMER_ONE_OFF,
};

/// @brief An interface that all timing sources must implement.
class IGenericClock
{
public:
  IGenericClock() = default; ///< Standard constructor
  virtual ~IGenericClock() = default; ///< Standard destructor

  /// @brief Return the current time, according to this clock.
  ///
  /// @param[out] time If the function is successful, stores the current time.
  ///
  /// @return True if the function succeeded, in which case 'time' is valid, or false otherwise.
  virtual bool get_current_time(time_expanded &time) = 0;
};

typedef void (*timer_callback)(void *);

void time_gen_init();

void time_sleep_process(uint64_t wait_in_ns);
void time_stall_process(uint64_t wait_in_ns);

uint64_t time_get_system_timer_count(bool output_in_ns = false);
uint64_t time_get_system_timer_offset(uint64_t wait_in_ns);

const unsigned int time_task_mgr_int_period_ns = 1000000;

bool time_register_clock_source(std::shared_ptr<IGenericClock> clock);
bool time_unregister_clock_source(std::shared_ptr<IGenericClock> clock);
bool time_get_current_time(time_expanded &time);
