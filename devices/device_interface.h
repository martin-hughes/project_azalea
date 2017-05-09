#ifndef CORE_DEVICE_INTFACE_HEADER
#define CORE_DEVICE_INTFACE_HEADER

#include "klib/data_structures/string.h"

enum class DEV_STATUS
{
  OK,
  FAILED,
  STOPPED,
  NOT_PRESENT,
};

class IDevice
{
public:
  virtual ~IDevice() = default;

  virtual const kl_string device_name() = 0;
  virtual DEV_STATUS get_device_status() = 0;
};

#endif
