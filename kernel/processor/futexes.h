/// @file
/// @brief Declare futex operations
///
/// See the Linux futex and robust futex documentation for a fuller description of how futexes work.

#pragma once

#include "stdint.h"
#include "user_interfaces/error_codes.h"

void futex_maybe_init();

ERR_CODE futex_wait(uint64_t phys_addr, int32_t cur_value, int32_t req_value);
ERR_CODE futex_wake(uint64_t phys_addr);
