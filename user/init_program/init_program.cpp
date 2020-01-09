// Azalea's initialization program.
//
// At present, all this does is start the shell program, repeatedly.

#include <azalea/azalea.h>
#include <string.h>

#define SC_DEBUG_MSG(string) \
  syscall_debug_output((string), strlen((string)) )

extern "C" int main (int argc, char **argv, char **env_p);

int main (int argc, char **argv, char **env_p)
{
  GEN_HANDLE proc_handle;

  SC_DEBUG_MSG("Azalea initialization program\n");

  while (1)
  {
    SC_DEBUG_MSG("Start shell\n");
    if (exec_file("\\root\\shell", 11, &proc_handle, nullptr, nullptr) != ERR_CODE::NO_ERROR)
    {
      SC_DEBUG_MSG("Failed to execute shell\n");
      break;
    }
    else
    {
      SC_DEBUG_MSG("Done\n");
      syscall_wait_for_object(proc_handle);
      syscall_close_handle(proc_handle);
      SC_DEBUG_MSG("Shell terminated - restart.\n");
    }
  }

  return 0;
}