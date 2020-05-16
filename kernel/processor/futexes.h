/// @file
/// @brief Declare futex operations
///
/// See the Linux futex and robust futex documentation for a fuller description of how futexes work.

#pragma once

#include "stdint.h"
#include "user_interfaces/error_codes.h"

ERR_CODE futex_wait(volatile int32_t *futex, int32_t req_value);
ERR_CODE futex_wake(volatile int32_t *futex);
