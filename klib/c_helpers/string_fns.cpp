/// @file
/// @brief Kernel Support Library string helper functions
///
/// These closely resemble some C-library string handling functions, but tweaked to make them more suitable or robust.

#include "klib/klib.h"

/// @brief Measure the length of a string
///
/// @param str The string to return the length of. A string is determined to be ended by a null character.
///
/// @param max_len The maximum distance to travel through memory looking. This can be used to prevent invalid memory
///                accesses. If the string is actually longer than max_len, max_len is returned. If max_len is zero,
///                no checking is performed.
///
/// @return The length of the string, or max_len if max_len is less than the length of the string and not zero.
unsigned long kl_strlen(const char *str, unsigned long max_len)
{
  unsigned long len = 0;

  while ((*str != 0) && ((max_len == 0) || (len < max_len)))
  {
    str++;
    len++;
  }

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
int kl_strcmp(const char *str_a, const unsigned long max_len_a, const char *str_b, const unsigned long max_len_b)
{
  unsigned long ctr = 0;
  
  while (1)
  {
    if (*str_a < *str_b)
    {
      return -1;
    }
    else if (*str_a > *str_b)
    {
      return 1;
    }

    // If the end of string is reached, the strings are equal.
    if (str_a == 0)
    {
      return 0;
    }

    ctr++;
    str_a++;
    str_b++;

    // If the maximum length is reached, the strings are considered equal.
    if (((ctr >= max_len_a) && (max_len_a != 0)) || ((ctr >= max_len_b) && (max_len_b != 0)))
    {
      return 0;
    }
  }
}