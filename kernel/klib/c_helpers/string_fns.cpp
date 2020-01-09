/// @file
/// @brief Kernel Support Library string helper functions
///
/// These closely resemble some C-library string handling functions, but tweaked to make them more suitable or robust.

//#define ENABLE_TRACING

#include "klib/klib.h"

#include <string.h>

/// @brief Measure the length of a string
///
/// @param str The string to return the length of. A string is determined to be ended by a null character.
///
/// @param max_len The maximum distance to travel through memory looking. This can be used to prevent invalid memory
///                accesses. If the string is actually longer than max_len, max_len is returned. If max_len is zero,
///                no checking is performed.
///
/// @return The length of the string, or max_len if max_len is less than the length of the string and not zero.
uint64_t kl_strlen(const char *str, uint64_t max_len)
{
  uint64_t len{0};

  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "String ptr: ", (void *)str, ", max_len: ", max_len, "\n");

  len = strnlen(str, max_len);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Length: ", len, "\n");
  KL_TRC_EXIT;

  return len;
}

/// @brief Compare two strings to determine which or the other comes first by binary comparison
///
/// Strings are considered to be terminated either by a NULL character, or by the comparison exceeding the maximum
/// lengths provided in either max_len_a or max_len_b (noting that zero values for either of these means that the
/// string could have infinite length). The comparison only continues to the smaller of the two max length values.
///
/// @param str_a The first string to compare
///
/// @param max_len_a The maximum length of a to consider.
///
/// @param str_b The second string to compare
///
/// @param max_len_b The maximum length of b to consider.
///
/// @return If str_a < str_b, -1. If str_a == str_b, 0. Otherwise +1.
int32_t kl_strcmp(const char *str_a, const uint64_t max_len_a, const char *str_b, const uint64_t max_len_b)
{
  uint64_t ctr{0};
  uint32_t result;

  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "String a ptr: ", (void *)str_a, ", max length: ", max_len_a, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "String b ptr: ", (void *)str_b, ", max length: ", max_len_b, "\n");

  while (1)
  {
    if (*str_a < *str_b)
    {
      result = -1;
      break;
    }
    else if (*str_a > *str_b)
    {
      result = 1;
      break;
    }

    // If the end of string is reached, the strings are equal.
    if (str_a == 0)
    {
      result = 0;
      break;
    }

    ctr++;
    str_a++;
    str_b++;

    // If the maximum length is reached, the strings are considered equal.
    if (((ctr >= max_len_a) && (max_len_a != 0)) || ((ctr >= max_len_b) && (max_len_b != 0)))
    {
      result = 0;
      break;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
