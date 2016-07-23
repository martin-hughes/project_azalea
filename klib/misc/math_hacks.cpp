#include "math_hacks.h"

// Round a number to the next-highest power of two. Do this by setting all bits
// below the one that will eventually be set to 1, since all ones is equivalent
// to ((2 ^ n) - 1), and then adding one to the result.
//
// Credit for this goes to the algorithm on this page:
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
unsigned int round_to_power_two(unsigned int input)
{
  input--;
  input |= input >> 1;
  input |= input >> 2;
  input |= input >> 4;
  input |= input >> 8;
  input |= input >> 16;
  input++;

  return input;
}
