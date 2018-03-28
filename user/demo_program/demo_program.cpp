#include "user_interfaces/syscall.h"
#include "user_interfaces/messages.h"

extern "C" int main();

unsigned char message[100] = "Couldn't load file...\n";
const char filename[] = "root\\text.txt";
const char terminal_path[] = "pipes\\terminal\\write";
unsigned char output_msg[] = "Hello - look at this shiny text!";

#define MESSAGE(name, text) \
  const char name [] = text; \
  const unsigned int name ## _len = sizeof(name)

#define SC_DEBUG_MSG(name) \
  syscall_debug_output(name, name ## _len - 1 )

MESSAGE(msg_hello, "Hello!\n");
MESSAGE(msg_no_open_handle, "Couldn't open handle\n");
MESSAGE(msg_no_size, "Couldn't determine size, assume 10.\n");
MESSAGE(msg_no_read, "Couldn't read from handle\n");
MESSAGE(msg_no_close, "Couldn't close handle\n");
MESSAGE(msg_getting_pipe, "Getting terminal pipe");
MESSAGE(msg_couldnt_write_term, "Couldn't write to terminal\n");
MESSAGE(msg_no_message, "Couldn't register for message passing\n");
MESSAGE(msg_keydown, "Keydown message\n");
MESSAGE(msg_keyup, "Keyup message\n");
MESSAGE(msg_pchar, "Printable character message\n");
MESSAGE(msg_didnt_complete, "Kernel message didn't complete\n");
MESSAGE(msg_unknown, "Unknown message\n");
MESSAGE(msg_wrong_msg_len, "Wrong message length\n");

void file_read_test();
GEN_HANDLE simple_term_write();
void keyboard_msgs(GEN_HANDLE term_handle);

int main()
{
  GEN_HANDLE term_handle;

  SC_DEBUG_MSG(msg_hello);

  // Start with a basic test of whether file handles work correctly.
  file_read_test();

  // Continue by displaying text on the screen! When trying to get a hold of the screen pipe it may not yet exist, so
  // spin until it does. Returns a handle to the terminal.
  term_handle = simple_term_write();

  // Deal with keyboard messages. This function won't actually return.
  keyboard_msgs(term_handle);

  return 0;
}

// Read from a file and write its contents to the system debug output.
void file_read_test()
{
  unsigned long message_len = 22;
  GEN_HANDLE handle;
  ERR_CODE result;
  unsigned long bytes_read;
  unsigned long file_size;

  result = syscall_open_handle(filename, sizeof(filename), &handle);
  if (result != ERR_CODE::NO_ERROR)
  {
    SC_DEBUG_MSG(msg_no_open_handle);
  }

  result = syscall_get_handle_data_len(handle, &file_size);
  if (result != ERR_CODE::NO_ERROR)
  {
    SC_DEBUG_MSG(msg_no_size);
    file_size = 10;
  }
  if (file_size > 100)
  {
    file_size = 100;
  }

  result = syscall_read_handle(handle, 0, file_size, message, 100, &bytes_read);
  if (result != ERR_CODE::NO_ERROR)
  {
    SC_DEBUG_MSG(msg_no_read);
  }

  syscall_debug_output((const char *)message, message_len);

  result = syscall_close_handle(handle);
  if (result != ERR_CODE::NO_ERROR)
  {
    SC_DEBUG_MSG(msg_no_close);
  }

  result = syscall_read_handle(handle, 0, 1, message, 100, &bytes_read);
  if (result == ERR_CODE::NO_ERROR)
  {
    SC_DEBUG_MSG(msg_no_read);
  }
}

// Open a pipe to the terminal process and write a string on it.
GEN_HANDLE simple_term_write()
{
  ERR_CODE result;
  GEN_HANDLE handle;
  unsigned long bytes_written;

  result = ERR_CODE::UNKNOWN;

  SC_DEBUG_MSG(msg_getting_pipe);
  while (result != ERR_CODE::NO_ERROR)
  {
    result = syscall_open_handle(terminal_path, sizeof(terminal_path), &handle);
    syscall_debug_output(".", 1);
  }
  syscall_debug_output("Done. \n", 7);

  result = syscall_write_handle(handle, 0, sizeof(output_msg), output_msg, sizeof(output_msg), &bytes_written);
  if (result != ERR_CODE::NO_ERROR)
  {
    SC_DEBUG_MSG(msg_couldnt_write_term);
  }

  return handle;
}

// Receive keyboard messages and write information about them.
void keyboard_msgs(GEN_HANDLE term_handle)
{
  ERR_CODE result;
  unsigned long sending_proc_id;
  unsigned long message_id;
  unsigned long message_len;
  keypress_msg key_updown;
  key_char_msg printable_msg;
  unsigned long bytes_written;

  result = syscall_register_for_mp();
  if (result != ERR_CODE::NO_ERROR)
  {
    SC_DEBUG_MSG(msg_no_message);
  }

  while(1)
  {
    // See if the keyboard sent any scan code messages.
    result = syscall_receive_message_details(sending_proc_id, message_id, message_len);

    if (result == ERR_CODE::NO_ERROR)
    {
      switch (message_id)
      {
        case SM_KEYDOWN:
          SC_DEBUG_MSG(msg_keydown);
          break;

        case SM_KEYUP:
          SC_DEBUG_MSG(msg_keyup);
          break;

        case SM_PCHAR:
          SC_DEBUG_MSG(msg_pchar);
          if (message_len != sizeof(key_char_msg))
          {
            SC_DEBUG_MSG(msg_wrong_msg_len);
          }
          else
          {
            syscall_receive_message_body(reinterpret_cast<char *>(&printable_msg), sizeof(printable_msg));

            output_msg[0] = printable_msg.pressed_character;

            result = syscall_write_handle(term_handle, 0, 1, output_msg, 1, &bytes_written);
            if (result != ERR_CODE::NO_ERROR)
            {
              SC_DEBUG_MSG(msg_couldnt_write_term);
            }
          }
          break;

        default:
          SC_DEBUG_MSG(msg_unknown);
      }

      result = syscall_message_complete();
      if (result != ERR_CODE::NO_ERROR)
      {
        SC_DEBUG_MSG(msg_didnt_complete);
      }
    }
  }
}