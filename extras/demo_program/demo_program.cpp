#include "syscall/syscall.h"

extern "C" int main();

unsigned char message[100] = "Couldn't load file...\n";
const char filename[] = "root\\text.txt";
const char terminal_path[] = "pipes\\terminal\\write";
unsigned char output_msg[] = "Hello - look at this shiny text!";

int main()
{
  unsigned long message_len = 22;
  GEN_HANDLE handle;
  ERR_CODE result;
  unsigned long bytes_read;
  unsigned long file_size;

  syscall_debug_output("Hello!\n", 7);

  // Start with a basic test of whether file handles work correctly.
  result = syscall_open_handle(filename, sizeof(filename), &handle);
  if (result != ERR_CODE::NO_ERROR)
  {
    syscall_debug_output("Couldn't open handle\n", 21);
  }

  result = syscall_get_handle_data_len(handle, &file_size);
  if (result != ERR_CODE::NO_ERROR)
  {
    syscall_debug_output("Couldn't determine size, assume 10.\n", 36);
    file_size = 10;
  }
  if (file_size > 100)
  {
    file_size = 100;
  }

  result = syscall_read_handle(handle, 0, file_size, message, 100, &bytes_read);
  if (result != ERR_CODE::NO_ERROR)
  {
    syscall_debug_output("Couldn't read from handle\n", 26);
  }

  syscall_debug_output((const char *)message, message_len);

  result = syscall_close_handle(handle);
  if (result != ERR_CODE::NO_ERROR)
  {
    syscall_debug_output("Couldn't close handle\n", 22);
  }

  result = syscall_read_handle(handle, 0, 1, message, 100, &bytes_read);
  if (result == ERR_CODE::NO_ERROR)
  {
    syscall_debug_output("Could read from handle!!\n", 25);
  }

  // Continue by displaying text on the screen! When trying to get a hold of the screen pipe it may not yet exist, so spin until it does.
  result = ERR_CODE::UNKNOWN;

  syscall_debug_output("Getting terminal pipe", 22);
  while (result != ERR_CODE::NO_ERROR)
  {
    result = syscall_open_handle(terminal_path, sizeof(terminal_path), &handle);
    syscall_debug_output(".", 2);
  }
  syscall_debug_output("\nDone.", 7);

  result = syscall_write_handle(handle, 0, sizeof(output_msg), output_msg, sizeof(output_msg), &bytes_read);
  if (result != ERR_CODE::NO_ERROR)
  {
    syscall_debug_output("Didn't write terminal\n", 22);
  }

  return 0;
}
