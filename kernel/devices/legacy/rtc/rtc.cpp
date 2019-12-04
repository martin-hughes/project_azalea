/// @file
/// @brief Driver for common RTC chips
//
// Doesn't even pay lip service to IDevice.

//#define ENABLE_TRACING

#include "rtc.h"
#include "klib/klib.h"

using namespace timing;

/// @brief Create a new RTC driver object.
///
/// @param obj_handle ACPI object handle for this device, used to get the CMOS control port to use.
///
/// @return A shared_ptr to the new RTC driver object.
std::shared_ptr<rtc> rtc::create(ACPI_HANDLE obj_handle)
{
  KL_TRC_ENTRY;

  auto r = std::shared_ptr<rtc>(new rtc(obj_handle));

  KL_TRC_EXIT;

  return r;
}

/// @brief Initialise a driver for a generic RTC.
///
/// @param obj_handle ACPI object handle for this device, used to get the CMOS control port to use.
rtc::rtc(ACPI_HANDLE obj_handle) : IDevice{"Real time clock", "rtc", true}
{
  ACPI_STATUS status;
  ACPI_BUFFER buf;
  uint8_t *raw_ptr;
  ACPI_RESOURCE *resource_ptr;
  uint16_t io_resources_found = 0;
  uint8_t status_b;

  KL_TRC_ENTRY;

  buf.Length = ACPI_ALLOCATE_BUFFER;
  buf.Pointer = nullptr;

  set_device_status(DEV_STATUS::STOPPED);

  // Iterate over all provided resources to find one which tells us the CMOS port. Probably it's 0x70...
  status = AcpiGetCurrentResources(obj_handle, &buf);
  if (status == AE_OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Successfully got current resources\n");
    raw_ptr = reinterpret_cast<uint8_t *>(buf.Pointer);
    resource_ptr = reinterpret_cast<ACPI_RESOURCE *>(raw_ptr);

    while ((resource_ptr->Length != 0) && (resource_ptr->Type != ACPI_RESOURCE_TYPE_END_TAG))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Resource type: ", resource_ptr->Type, "\n");
      switch (resource_ptr->Type)
      {
      case ACPI_RESOURCE_TYPE_IO:
        KL_TRC_TRACE(TRC_LVL::FLOW, "IO resources\n");

        switch(io_resources_found)
        {
        case 0:
          KL_TRC_TRACE(TRC_LVL::FLOW, "Found CMOS port resource\n"); // Probably...
          cmos_base_port = resource_ptr->Data.Io.Minimum;
          if (resource_ptr->Data.Io.Minimum != resource_ptr->Data.Io.Maximum)
          {
            // I'm interested, but not enough to do anything about it!
            KL_TRC_TRACE(TRC_LVL::IMPORTANT, "Unknown extra IO ports on RTC ACPI table...\n");
          }
          break;

        default:
          KL_TRC_TRACE(TRC_LVL::FLOW, "Found more resources, currently unused.\n");
          break;
        }

        io_resources_found++;

        break;

      case ACPI_RESOURCE_TYPE_IRQ:
      case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
        // Azalea doesn't currently support RTC interrupts.
        KL_TRC_TRACE(TRC_LVL::FLOW, "IRQ list\n");
        break;
      }

      raw_ptr += resource_ptr->Length;
      resource_ptr = reinterpret_cast<ACPI_RESOURCE *>(raw_ptr);
    }

    set_device_status(DEV_STATUS::OK);
    KL_TRC_TRACE(TRC_LVL::FLOW, "Using CMOS port: ", cmos_base_port, "\n");

    status_b = read_cmos_byte(CMOS_RTC_REGISTERS::STATUS_B);
    bcd_mode = ((status_b & 0x04) == 0);
    am_pm_mode = ((status_b & 0x02) == 0);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to get resources\n");
    set_device_status(DEV_STATUS::FAILED);
  }

  if (buf.Pointer != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Release buffer\n");
    AcpiOsFree(buf.Pointer);
  }

  KL_TRC_EXIT;
}

bool rtc::start()
{
  set_device_status(DEV_STATUS::OK);
  return true;
}

bool rtc::stop()
{
  set_device_status(DEV_STATUS::STOPPED);
  return true;
}

bool rtc::reset()
{
  set_device_status(DEV_STATUS::STOPPED);
  return true;
}

/// @cond
#define DECODE_BCD_BYTE(x) (((x) & 0x0F) + ((((x) & 0xF0) >> 4) * 10))
/// @endcond

bool rtc::get_current_time(time_expanded &time)
{
  bool result{true};
  uint8_t raw_s[2];
  uint8_t raw_m[2];
  uint8_t raw_h[2];
  uint8_t raw_D[2];
  uint8_t raw_M[2];
  uint8_t raw_Y[2];

  KL_TRC_ENTRY;

  // Wait for update flag to be clear.
  while ((read_cmos_byte(CMOS_RTC_REGISTERS::STATUS_A) & 0x80) != 0) { };

  // Read everything twice AND check the update flag doesn't become set while we're looking at the clock. A combination
  // of both should ensure that we don't get caught out by the clock updating. It should also catch the case where we
  // get interrupted and someone else reads a different CMOS byte...
  raw_s[0] = read_cmos_byte(CMOS_RTC_REGISTERS::SECONDS);
  raw_m[0] = read_cmos_byte(CMOS_RTC_REGISTERS::MINUTES);
  raw_h[0] = read_cmos_byte(CMOS_RTC_REGISTERS::HOURS);
  raw_D[0] = read_cmos_byte(CMOS_RTC_REGISTERS::DAY_OF_MONTH);
  raw_M[0] = read_cmos_byte(CMOS_RTC_REGISTERS::MONTH);
  raw_Y[0] = read_cmos_byte(CMOS_RTC_REGISTERS::YEAR);

  raw_s[1] = read_cmos_byte(CMOS_RTC_REGISTERS::SECONDS);
  raw_m[1] = read_cmos_byte(CMOS_RTC_REGISTERS::MINUTES);
  raw_h[1] = read_cmos_byte(CMOS_RTC_REGISTERS::HOURS);
  raw_D[1] = read_cmos_byte(CMOS_RTC_REGISTERS::DAY_OF_MONTH);
  raw_M[1] = read_cmos_byte(CMOS_RTC_REGISTERS::MONTH);
  raw_Y[1] = read_cmos_byte(CMOS_RTC_REGISTERS::YEAR);

  // An update started while we were doing that...
  if (((read_cmos_byte(CMOS_RTC_REGISTERS::STATUS_A) & 0x80) != 0) ||
      (raw_s[0] != raw_s[1]) ||
      (raw_m[0] != raw_m[1]) ||
      (raw_h[0] != raw_h[1]) ||
      (raw_D[0] != raw_D[1]) ||
      (raw_M[0] != raw_M[1]) ||
      (raw_Y[0] != raw_Y[1]))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Caught mid-update, try again\n");
    result = get_current_time(time);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Read CMOS clock, now decode\n");
    if (bcd_mode)
    {
      time.nanoseconds = 0;
      time.seconds = DECODE_BCD_BYTE(raw_s[0]);
      time.minutes = DECODE_BCD_BYTE(raw_m[0]);
      if (am_pm_mode)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "AM/PM mode with flag\n");
        time.hours = DECODE_BCD_BYTE(raw_h[0] & 0x7F) + (((raw_h[0] & 0x80) != 0) ? 12 : 0);
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "24-hour clock mode\n");
        time.hours = DECODE_BCD_BYTE(raw_h[0]);
      }
      time.day = DECODE_BCD_BYTE(raw_D[0]);
      time.month = DECODE_BCD_BYTE(raw_M[0]);
      time.year = 2000 + DECODE_BCD_BYTE(raw_Y[0]);
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Non BCD decode\n");
      time.nanoseconds = 0;
      time.seconds = raw_s[0];
      time.minutes = raw_m[0];
      if (am_pm_mode)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "AM/PM mode with flag\n");
        time.hours = (raw_h[0] & 0x7F) + (((raw_h[0] & 0x80) != 0) ? 12 : 0);
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "24-hour clock mode\n");
        time.hours = raw_h[0];
      }
      time.day = raw_D[0];
      time.month = raw_M[0];
      time.year = 2000 + raw_Y[0];
    }

  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Read a single byte from the given CMOS register.
///
/// @param reg Which register to read.
///
/// @return The byte that was read.
uint8_t rtc::read_cmos_byte(CMOS_RTC_REGISTERS reg)
{
  uint8_t result;
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::FLOW, "Reading CMOS register: ", static_cast<uint16_t>(reg), "\n");

  proc_write_port(cmos_base_port, static_cast<uint16_t>(reg), 8);
  result = proc_read_port(cmos_base_port + 1, 8);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");

  return result;
}