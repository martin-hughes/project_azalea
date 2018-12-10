#include <stdio.h>

extern "C" int main (int argc, char **argv);

int main (int argc, char **argv)
{
  for (int i = 1; i < argc; i++)
  {
    if (i != 1)
    {
      printf(" ");
    }
    printf("%s", argv[i]);
  }
  printf("\n");

  return 0;
}