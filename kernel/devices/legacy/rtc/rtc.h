/// @file
/// @brief Driver for most RTC chips.

#pragma once

#include "devices/device_interface.h"
#include "acpi/acpi_if.h"
#include "processor/timing/timing.h"
#include <memory>

namespace timing
{

enum class CMOS_RTC_REGISTERS : uint16_t
{
  SECONDS = 0,
  MINUTES = 2,
  HOURS = 4,
  WEEKDAY = 6,
  DAY_OF_MONTH = 7,
  MONTH = 8,
  YEAR = 9,
  STATUS_A = 10,
  STATUS_B = 11,
  CENTURY = 50,
};

class rtc : public IDevice, public IGenericClock
{
public:
  static std::shared_ptr<rtc> create(ACPI_HANDLE obj_handle);
protected:
  rtc(ACPI_HANDLE obj_handle);
public:
  virtual ~rtc() = default; ///< Default destructor

  // Overrides from IGenericClock
  virtual bool get_current_time(time_expanded &time) override;

protected:
  uint16_t cmos_base_port{0x70}; ///< The CMOS port to use when reading values for this RTC.
  bool bcd_mode{false}; ///< If true, the clock stores BCD digits.
  bool am_pm_mode{false}; ///< If true, the clock stores hours as AM/PM with a flag for PM.

  uint8_t read_cmos_byte(CMOS_RTC_REGISTERS reg);
};

} // namespace timing
