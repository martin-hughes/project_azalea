#include "buffers.h"

// TODO: Add sanity checking of ranges. (STAB)
// TODO: This is probably not meant to just set 0... (BUG)
void kl_memset(void* buffer, unsigned char* val, unsigned long len)
{
	unsigned char* b = (unsigned char*)buffer;
	for(unsigned long done = 0; done < len; done++)
	{
		b[done] = 0;
	}
}

// TODO: Add sanity checking of ranges! (STAB)
void kl_memcpy(void *from, void *to, unsigned long len)
{
  unsigned char *f = (unsigned char *)from;
  unsigned char *t = (unsigned char *)to;

  for (unsigned long i = 0; i < len; i++)
  {
    *t = *f;
    t++;
    f++;
  }
}
