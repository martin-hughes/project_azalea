#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "azalea/syscall.h"

extern "C" int main (int argc, char **argv);

int main (int argc, char **argv)
{
  ERR_CODE ec;
  GEN_HANDLE folder_handle;

  for (int i = 1; i < argc; i++)
  {
    printf("%s: ", argv[i]);

    ec = az_open_handle(argv[i], strlen(argv[i]), &folder_handle, 0);
    if (ec == ERR_CODE::NO_ERROR)
    {
      uint64_t num_reqd{0};
      printf("Found.\n");

      ec = az_enum_children(folder_handle, nullptr, 0, 0, nullptr, &num_reqd);
      if (ec == ERR_CODE::NO_ERROR)
      {
        char *buf = reinterpret_cast<char *>(malloc(num_reqd));
        ec = az_enum_children(folder_handle, nullptr, 0, 0, buf, &num_reqd);
        if (ec == ERR_CODE::NO_ERROR)
        {
          char **ptr_table = reinterpret_cast<char **>(buf);
          uint64_t idx{0};

          while (ptr_table[idx] != nullptr)
          {
            printf(" - %s\n", ptr_table[idx]);

            idx++;
          }
        }
        else
        {
          printf(" - could enumerate children but failed to get names\n");
        }
        free(buf);
        buf = nullptr;
      }
      else
      {
        printf(" - unable to enumerate children\n");
      }

    }
    else if (ec == ERR_CODE::NOT_FOUND)
    {
      printf("Not found.\n");
    }
    else
    {
      printf("ERROR.\n");
    }
    az_close_handle(folder_handle);
  }
  printf("\n");

  return 0;
}