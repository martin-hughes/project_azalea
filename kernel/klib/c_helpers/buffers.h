#ifndef _AZALEA_BUFFERS
#define _AZALEA_BUFFERS

// Contains simple helper functions for dealing with buffers - string-based or
// otherwise.

void kl_memset(void* buffer, unsigned char val, unsigned long len);
void kl_memcpy(const void *from, void *to, unsigned long len);
char kl_memcmp(const void *a, const void *b, unsigned long len);

// Don't include memmove in the test code, it just causes wobblies.
#ifndef AZALEA_TEST_CODE
extern "C" void *memmove(void *dest, const void *src, unsigned long length);
#endif

#endif