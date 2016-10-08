// Contains simple helper functions for dealing with buffers - string-based or
// otherwise.

void kl_memset(void* buffer, unsigned char val, unsigned long len);
void kl_memcpy(const void *from, void *to, unsigned long len);
char kl_memcmp(const void *a, const void *b, unsigned long len);
