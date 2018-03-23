#include "math_hacks.h"

// Round a number to the next-highest power of two.
unsigned long round_to_power_two(unsigned long input)
{
  unsigned long pos;
  unsigned long output;
  unsigned long first_in = input;

  if (input == 0)
  {
    return 0;
  }

  if ((input & 0x1000000000000000) != 0)
  {
    return 0;
  }

  pos = __builtin_clzl(input);
  output = (unsigned long)1 << (63 - pos);

  if (first_in != output)
  {
    output = output << 1;
  }

  return output;
}
