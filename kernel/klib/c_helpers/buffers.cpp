/// @file
/// @brief Kernel memory buffer helper functions

#include <stdint.h>

#include "buffers.h"
#include "klib/tracing/tracing.h"
#include "klib/panic/panic.h"
#include "klib/misc/assert.h"

/// @brief Kernel memory setting function
///
/// A drop-in replacement for the familiar memset function. The entire buffer must be contained within kernel memory
/// (i.e. the upper half of virtual memory space) and must not wrap.
///
/// @param buffer The buffer whose value is set to a repetition of val.
///
/// @param val This character is repetitively filled across buffer.
///
/// @param len The length of the buffer to fill with the value in val
void kl_memset(void* buffer, uint8_t val, uint64_t len)
{
  uint64_t end = reinterpret_cast<uint64_t>(buffer) + len;

  // If end is less than buffer, then that suggests that the mem-setting would wrap back in to user space.
  ASSERT(end > reinterpret_cast<uint64_t>(buffer));

  // Make sure this setting occurs in kernel space, by checking that the high bit of the address is set.
#ifndef AZALEA_TEST_CODE
  ASSERT((reinterpret_cast<uint64_t>(buffer) & (((uint64_t)1) << 63)) != 0);
#endif

	unsigned char* b = (unsigned char*)buffer;
	for(unsigned long done = 0; done < len; done++)
	{
		b[done] = val;
	}
}

/// @brief Kernel buffer copying function
///
/// A drop in replacement for the familiar memcpy function.
///
/// Buffers must not wrap the end of memory, and must be contained entirely within either the kernel or user space
/// parts of virtual memory. They must not overlap both parts.
///
/// It is the caller's responsibility to ensure that the destination buffer is large enough for the copying to occur.
///
/// @param from The buffer to copy from
///
/// @param to The buffer to copy to
///
/// @param len The length of data to copy.
void kl_memcpy(const void *from, void *to, uint64_t len)
{
  uint8_t *f = (uint8_t *)from;
  uint8_t *t = (uint8_t *)to;
  uint64_t from_end = (uint64_t)from + len;
  uint64_t to_end = (uint64_t)to + len;

  // If length is zero, don't bother doing anything - we might as well bail out now. This also avoids any of the checks
  // below triggering.
  if (len == 0)
  {
    return;
  }

  // Make sure that the copying doesn't wrap.
  ASSERT(from_end > (uint64_t)from);
  ASSERT(to_end > (uint64_t)to);

  // Ensure that the buffers are contained entirely within user space if they start there. If they start in kernel
  // space then the ASSERTs above prevent them wrapping back to overlap userspace.
  if ((uint64_t)from < 0x8000000000000000)
  {
    ASSERT(from_end < 0x8000000000000000);
  }
  if ((uint64_t)to < 0x8000000000000000)
  {
    ASSERT(to_end < 0x8000000000000000);
  }

  for (uint64_t i = 0; i < len; i++)
  {
    *t = *f;
    t++;
    f++;
  }
}

/// @brief Kernel buffer comparison function
///
/// Approximately a drop-in for regular memcmp, compares two buffers and returns which of them (if either) is lower
/// numerically
///
/// @param a The first buffer to compare
///
/// @param b The second buffer to compare
///
/// @param len The maximum number of bytes to compare.
///
/// @return 0 if the buffers are equal, -1 if a < b and +1 if a > b
int8_t kl_memcmp(const void *a, const void *b, uint64_t len)
{
  const uint8_t *_a = static_cast<const uint8_t *>(a);
  const uint8_t *_b = static_cast<const uint8_t *>(b);

  uint64_t ctr = 0;

  if (len == 0)
  {
    return 0;
  }

  while (ctr < len)
  {
    if (*_a < *_b)
    {
      return -1;
    }
    else if (*_a > *_b)
    {
      return 1;
    }

    ctr++;
    _a++;
    _b++;
  }

  return 0;
}

// Don't include memmove in the test code, it just causes wobblies.
#ifndef AZALEA_TEST_CODE
/// @cond
// I'm not going to document this - it has the standard meaning and should be removed soon.
void *memmove(void *dest, const void *src, uint64_t length)
{
  char *temp = new char[length];

  kl_memcpy(src, temp, length);
  kl_memcpy(temp, dest, length);

  delete[] temp;

  return dest;
}
/// @endcond
#endif