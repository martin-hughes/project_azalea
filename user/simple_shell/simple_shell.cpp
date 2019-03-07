#include <azalea/azalea.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>

#define SC_DEBUG_MSG(string) \
  syscall_debug_output((string), strlen((string)) )

extern "C" int main (int argc, char **argv, char **env_p);

// #define OUTPUT_PARSE_RESULTS

char **child_argv = nullptr;
uint64_t child_argv_size = 0;
char *child_args_buffer = nullptr;
uint64_t child_args_buffer_size = 0;

bool is_command_char(char c);
void execute_command(char *command);
bool parse_command(char *command,
                   char** &argv_table,
                   char * &argv_buffer,
                   uint64_t &argv_table_size,
                   uint64_t argv_buffer_size);
bool count_command_details(char *command, uint64_t &num_args, uint64_t &argument_space_reqd);

int main (int argc, char **argv, char **env_p)
{
  SC_DEBUG_MSG("Welcome to simple shell\n");

  char *command_buffer;
  uint8_t buffer_offset = 0;
  int version = azalea_version();
  size_t command_len;
  const uint8_t MAX_CMD_LEN = 80;
  size_t result_len;
  time_expanded t;
  ERR_CODE result;

  printf("Azalea simple shell. OS Version: %d\n", version);

  command_buffer = (char *)malloc(MAX_CMD_LEN + 1);

  // Main command loop
  while (1)
  {
    //time_t rawtime;
    //struct tm * timeinfo;

    //time (&rawtime);
    //timeinfo = localtime (&rawtime);
    //printf("%02u:%02u:%02u ", (unsigned int)timeinfo->tm_hour, (unsigned int)timeinfo->tm_min, (unsigned int)timeinfo->tm_sec);

    result = syscall_get_system_clock(&t);
    if (result == ERR_CODE::NO_ERROR)
    {
      printf("%02u:%02u:%02u ", (unsigned int)t.hours, (unsigned int)t.minutes, (unsigned int)t.seconds);
    }
    else
    {
      printf("--:--:-- ");
    }
    printf("> ");
    fflush(stdout);

    buffer_offset = 0;
    memset(command_buffer, 0, sizeof(MAX_CMD_LEN + 1));
    result_len = MAX_CMD_LEN;

    command_len = getline(&command_buffer, &result_len, stdin);
    command_buffer[command_len - 1] = 0;

    if (command_len > 0 )
    {
      execute_command(command_buffer);
    }
    else
    {
      printf("Abort command\n");
    }
  };

  return 0;
}

bool is_command_char(char c)
{
  return (((c >= 'a') && (c <= 'z')) ||
          ((c >= 'A') && (c <= 'Z')) ||
          ((c >= '0') && (c <= '9')) ||
          (c == '\\') ||
          (c == ' '));
}

void execute_command(char *command)
{
  GEN_HANDLE proc_handle;

#ifdef OUTPUT_PARSE_RESULTS
  printf("Execute: %s\n", command);
#endif
  ERR_CODE result;
  int i;

  if (strlen(command) == 0)
  {
    printf("No command entered\n");
  }
  else
  {
    if (!parse_command(command, child_argv, child_args_buffer, child_argv_size, child_args_buffer_size))
    {
      printf("Unable to parse command\n");
    }
    else
    {
      i = 0;

#ifdef OUTPUT_PARSE_RESULTS
      while(child_argv[i] != nullptr)
      {
        printf("Arg %d: %s\n", i, child_argv[i]);
        i++;
      }
#endif

      if (strcmp(command, "exit") == 0)
      {
        printf("Exiting.\n");
        exit(0);
      }
      else
      {
        result = exec_file(child_argv[0],
                           strlen(child_argv[0]),
                           &proc_handle,
                           (char * const *)(&child_argv[1]),
                           nullptr);
        if (result != ERR_CODE::NO_ERROR)
        {
          printf("Command not found\n");
        }
        else
        {
          syscall_wait_for_object(proc_handle);
          syscall_close_handle(proc_handle);
        }
      }
    }
  }
}

bool parse_command(char *command,
                   char** &argv_table,
                   char * &argv_buffer,
                   uint64_t &argv_table_size,
                   uint64_t argv_buffer_size)
{
  uint64_t reqd_space = 0;
  uint64_t num_args = 0;
  char *write_ptr;
  uint64_t cur_arg;
  bool is_in_quote = false;

  count_command_details(command, num_args, reqd_space);

#ifdef OUTPUT_PARSE_RESULTS
  printf("Num args: %d, space: %d\n", num_args, reqd_space);
#endif

  if ((num_args + 1) > argv_table_size)
  {
    if (argv_table != nullptr)
    {
      free(argv_table);
    }
    argv_table = reinterpret_cast<char **>(malloc(sizeof(char *) * (num_args + 1)));
    argv_table_size = num_args + 1;
  }

  if (reqd_space > argv_buffer_size)
  {
    if (argv_buffer != nullptr)
    {
      free(argv_buffer);
    }
    argv_buffer = reinterpret_cast<char *>(malloc(reqd_space));
    argv_buffer_size = reqd_space;
  }

  write_ptr = argv_buffer;
  argv_table[0] = write_ptr;
  argv_table[num_args] = nullptr;
  cur_arg = 0;

  // Skip leading spaces.
  while (*command == ' ')
  {
    command++;
  }

  while (*command != 0)
  {
    *(write_ptr + 1) = 0;
    if (*command == '\'')
    {
      if (*(command + 1) == '\'')
      {
        *write_ptr = '\'';
        command++;
        write_ptr++;
      }
      else
      {
        is_in_quote = !is_in_quote;
      }
      command++;
    }
    else
    {
      if ((*command == ' ') && !is_in_quote)
      {
        // Skip consecutive spaces.
        while (*(command + 1) == ' ')
        {
          command++;
        }

        // Strip trailing spaces.
        if (*(command + 1) == 0)
        {
          break;
        }

        // Otherwise begin a new argument.
        *write_ptr = 0;
        write_ptr++;

        command++;
        cur_arg++;
        argv_table[cur_arg] = write_ptr;
      }
      else
      {
        *write_ptr = *command;
        write_ptr++;
        command++;
      }
    }
  }

  return !is_in_quote;
}

bool count_command_details(char *command, uint64_t &num_args, uint64_t &argument_space_reqd)
{
  bool is_in_quote = false;

  argument_space_reqd = 1; // Add an extra one to allow a trailing 0.
  num_args = 1;

  if (command == nullptr)
  {
    return false;
  }

  // Skip leading spaces.
  while (*command == ' ')
  {
    command++;
  }

  while(*command != 0)
  {
    if(*command == '\'')
    {
      if (*(command + 1) == '\'')
      {
        argument_space_reqd++;
        command++;
      }
      else
      {
        is_in_quote = !is_in_quote;
      }
    }
    else
    {
      if ((*command == ' ') && !is_in_quote)
      {
        // Skip consecutive spaces.
        while (*(command + 1) == ' ')
        {
          command++;
        }

        // Ignore trailing spaces.
        if (*(command + 1) == 0)
        {
          break;
        }

        num_args++;
        argument_space_reqd++;
      }
      else
      {
        argument_space_reqd++;
      }
    }

    command++;
  }

  if (is_in_quote)
  {
#ifdef OUTPUT_PARSE_RESULTS
    printf("Unmatched quote symbols\n");
#endif
    return false;
  }

  return true;
}