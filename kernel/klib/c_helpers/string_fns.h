/// @file
/// @brief Declare kernel-specific string helper functions.

#pragma once

#include <stdint.h>

uint64_t kl_strlen(const char *str, uint64_t max_len);
int32_t kl_strcmp(const char *str_a, const uint64_t max_len_a, const char *str_b, const uint64_t max_len_b);
