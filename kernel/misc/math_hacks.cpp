/// @file
/// @brief Simple math functions that are useful.

#include "math_hacks.h"

#ifdef _MSVC_LANG
#include <intrin.h>
#endif

/// @brief Round a number to the next-highest power of two.
///
/// @param input The number to round up.
///
/// @return Input rounded up to the next highest power of two - or itself, if input is a power of two already.
uint64_t round_to_power_two(uint64_t input)
{
  uint64_t pos;
  uint64_t output;
  uint64_t first_in = input;

  if (input == 0)
  {
    return 0;
  }

  if ((input & 0x1000000000000000) != 0)
  {
    return 0;
  }

#ifndef _MSVC_LANG
  pos = __builtin_clzl(input);
#else
  pos = __lzcnt64(input);
#endif
  output = (uint64_t)1 << (63 - pos);

  if (first_in != output)
  {
    output = output << 1;
  }

  return output;
}

/// @brief Which power of two is the greatest one still lower than this number?
///
/// That is, for any given input, the result is the maximum value of x where 2^x is less than, or equal to input. Or in
/// other words, log (input) / log(2) rounded down to the next integer.
///
/// @param input The number to convert.
///
/// @return The value described above.
uint64_t which_power_of_two(uint64_t input)
{
  uint64_t first_bit_pos;

#ifndef _MSVC_LANG
  first_bit_pos = __builtin_clzl(input);
#else
  first_bit_pos = __lzcnt64(input);
#endif

  return (input == 0) ? 0 : (63 - first_bit_pos);
}