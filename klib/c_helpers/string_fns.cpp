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
