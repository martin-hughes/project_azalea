/// @file
/// @brief Simple helper functions for dealing with buffers.

#include <stdint.h>

void kl_memset(void* buffer, uint8_t val, uint64_t len);
void kl_memcpy(const void *from, void *to, uint64_t len);
int8_t kl_memcmp(const void *a, const void *b, uint64_t len);

// Don't include memmove in the test code, it just causes wobblies.
#ifndef AZALEA_TEST_CODE
/// @cond
extern "C" void *memmove(void *dest, const void *src, uint64_t length);
/// @endcond
#endif
