/// @file
/// @brief Declare the panic function.

#pragma once

#include <stdint.h>

[[noreturn]] void panic(const char *message,
                        bool override_address = false,
                        uint64_t k_rsp = 0,
                        uint64_t r_rip = 0,
                        uint64_t r_rsp = 0);
