#include "syscall/syscall.h"

extern "C" int main();

const char message[] = "Hello, world!\n";

int main()
{
  while (1)
  {
    syscall_debug_output(message, 14);
  }
}
