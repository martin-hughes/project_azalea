#include <azalea/syscall.h>
#include <azalea/messages.h>
#include <azalea/system_properties.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#define SC_DEBUG_MSG(string) \
  syscall_debug_output((string), strlen((string)) )

extern "C" int main (int argc, char **argv, char **env_p);

int main (int argc, char **argv, char **env_p)
{
  SC_DEBUG_MSG("Hello!\n");

  FILE *f;
  const char *hw = "Hello, world!";

  if (argc != 2)
  {
    SC_DEBUG_MSG("Wrong argc\n");
  }

  syscall_debug_output(argv[0], strlen(argv[0]));

  char buf[60];
  unsigned long fs_reg = 5;

  __asm__ __volatile__ ("mov %%fs:0, %0" : "=r" (fs_reg) );

  snprintf(buf, 60, "ooooh %d\n", fs_reg);
  syscall_debug_output(buf, strlen(buf));

  f = fopen("root\\text.txt", "r");
  if (f == NULL)
  {
    SC_DEBUG_MSG("Couldn't open file\n");
  }
  else
  {
    fgets(buf, 60, f);
    SC_DEBUG_MSG("Read input: ");
    SC_DEBUG_MSG(buf);
    SC_DEBUG_MSG("\nDone.\n");

    fclose(f);
    f = NULL;
  }

  printf("Hello, world!\n");

  while(1) { };

  return 0;
}