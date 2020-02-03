/// @file
/// @brief Implements a kernel-stopping panic function.

#define ENABLE_TRACING

#include "panic.h"
#include "processor/processor.h"

#include <stdlib.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include <chrono>
#include <__external_threading>

void panic_print(const char *message, uint16_t line);
void panic_clear_screen();

char *vidmem = (char *)0xFFFFFFFF000b8000; ///< VGA 80x24 text mode buffer in kernel memory space.
const uint16_t screen_lines = 25; ///< The number of lines on the screen.
const uint16_t screen_cols = 80; ///< The number of columns on the screen.

namespace
{

  /// @brief Construct a libunwind context from a saved interrupt stack.
  ///
  /// The idea is to be able to feed this context to libunwind so as to see the call stack leading up to an interrupt.
  ///
  /// @param[out] context Place to save context in to.
  ///
  /// @param k_rsp Value of RSP register after kernel save stack process complete and passed to exception or fault
  ///              handler.
  ///
  /// @param r_rip Value of RIP in interrupted process.
  ///
  /// @param r_rsp Value of RSP in interrupted process.
  void generate_unw_context(unw_context_t &context, uint64_t k_rsp, uint64_t r_rip, uint64_t r_rsp)
  {
    memset(&context, 0, sizeof(context));

    uint64_t *stack_ptr = reinterpret_cast<uint64_t *>(k_rsp);
    uint64_t *context_ptr = reinterpret_cast<uint64_t *>(&context);

    KL_TRC_TRACE(TRC_LVL::FLOW, "Generating context from k_rsp: ", k_rsp, "\n");

    context_ptr[0] = stack_ptr[14]; // RAX
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved RAX: ", context_ptr[0], "\n");
    context_ptr[1] = stack_ptr[13]; // RBX
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved RBX: ", context_ptr[1], "\n");
    context_ptr[2] = stack_ptr[12]; // RCX
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved RCX: ", context_ptr[2], "\n");
    context_ptr[3] = stack_ptr[11]; // RDX
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved RDX: ", context_ptr[3], "\n");
    context_ptr[4] = stack_ptr[9]; // RDI
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved RDI: ", context_ptr[4], "\n");
    context_ptr[5] = stack_ptr[10]; // RSI
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved RSI: ", context_ptr[5], "\n");
    context_ptr[6] = stack_ptr[8]; // RBP
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved RBP: ", context_ptr[6], "\n");
    context_ptr[7] = r_rsp;
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved RSP: ", context_ptr[7], "\n");
    context_ptr[8] = stack_ptr[7]; // R8
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved R8: ", context_ptr[8], "\n");
    context_ptr[9] = stack_ptr[6]; // R9
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved R9: ", context_ptr[9], "\n");
    context_ptr[10] = stack_ptr[5]; // R10
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved R10: ", context_ptr[10], "\n");
    context_ptr[11] = stack_ptr[4]; // R11
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved R11: ", context_ptr[11], "\n");
    context_ptr[12] = stack_ptr[3]; // R12
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved R12: ", context_ptr[12], "\n");
    context_ptr[13] = stack_ptr[2]; // R13
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved R13: ", context_ptr[13], "\n");
    context_ptr[14] = stack_ptr[1]; // R14
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved R14: ", context_ptr[14], "\n");
    context_ptr[15] = stack_ptr[0]; // R15
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved R15: ", context_ptr[15], "\n");
    context_ptr[16] = r_rip;
    KL_TRC_TRACE(TRC_LVL::FLOW, "Saved RIP: ", context_ptr[16], "\n");
  }

  /// @brief Convert an integer value in to a string
  ///
  /// Don't use the library functions to avoid bringing in any dependencies.
  ///
  /// @param buf Buffer to put the converted results in to. Must be long enough.
  ///
  /// @param value The value to convert to string.
  void panic_convert_value(char *buf, uint64_t value)
  {
    memcpy(buf, "0x0000000000000000", 18);
    char temp = 0;

    for (int i = 0; i < 16; i++)
    {
      temp = (char)(value & 0x0F);
      if (temp < 10)
      {
        temp = '0' + temp;
      }
      else
      {
        temp = 'A' + (temp - 10);
      }
      buf[17 - i] = temp;
      value >>= 4;
    }
  }

  /// @brief Write a line of backtrace to the output
  ///
  /// @param rip RIP register for this line of backtrace.
  ///
  /// @param rsp RSP register for this line of backtrace.
  ///
  /// @param line Line to print this onto on the screen (not the backtrace line number)
  void write_backtrace_line(uint64_t rip, uint64_t rsp, uint8_t line)
  {
    char buffer[80];

    memset(buffer, 0, 80);
    strcpy(buffer, "RIP: ------------------, RSP: ------------------");
    panic_convert_value(buffer + 5, rip);
    panic_convert_value(buffer + 30, rsp);

    panic_print(buffer, line);
  }
}

/// @brief Implement an assertion failure function that's referenced in the C and C++ libraries.
///
/// @param expr The failed expression
///
/// @param file The filename of the file where the failure occurred.
///
/// @param line The line where the failure occurred.
///
/// @param func Name of the function where the failure occurred.
_Noreturn void __assert_fail(const char *expr, const char *file, int line, const char *func)
{
  // For now, ignore the other parameters.
  panic(expr);
}

/// @brief As expected, print a kernel panic message on to the screen, and stop running.
///
/// @param message The message to output.
///
/// @param override_address If set to true, use the values of k_rsp, r_rip and r_rsp to generate a backtrace to print
///                         alongside the panic message. If false, ignore those values and generate from the current
///                         context.
///
/// @param k_rsp Value of RSP register after kernel save stack process complete and passed to exception or fault
///              handler.
///
/// @param r_rip Value of RIP in failed process.
///
/// @param r_rsp Value of RSP in failed process.
[[noreturn]] void panic(const char *message,
                        bool override_address,
                        uint64_t k_rsp,
                        uint64_t r_rip,
                        uint64_t r_rsp)
{
  // Stop interrupts reaching this processor, in order to prevent most race
  // conditions.
  proc_stop_interrupts();

  // Stop all other processors too. It's possible that a another processor
  // could panic at the same time as this one, but we'll live with that race.
  proc_stop_other_procs();

  unw_cursor_t cursor; unw_context_t uc;
  unw_word_t ip, sp;
  bool good_context{false};

  if (override_address)
  {
    generate_unw_context(uc, k_rsp, r_rip, r_rsp);
    good_context = true;
  }
  else
  {
    if (unw_getcontext(&uc) == 0)
    {
      good_context = true;
    }
  }

  // Print a simple message on the screen.
  panic_clear_screen();
  panic_print("KERNEL PANIC", 0);
  panic_print("------------", 1);
  panic_print(message, 3);

  if (good_context && (unw_init_local(&cursor, &uc) == 0))
  {
    unw_get_reg(&cursor, UNW_REG_IP, &ip);
    unw_get_reg(&cursor, UNW_REG_SP, &sp);
    uint8_t line = 6;

    panic_print("Backtrace:", 5);
    write_backtrace_line(ip, sp, line);
    line++;

    while ((unw_step(&cursor) > 0) && (line < 25))
    {
      unw_get_reg(&cursor, UNW_REG_IP, &ip);
      unw_get_reg(&cursor, UNW_REG_SP, &sp);
      write_backtrace_line(ip, sp, line);

      line++;
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unable to generate backtrace\n");
    panic_print("No backtrace available", 5);
  }

  // Finally, we don't want to continue. This processor should be the last one
  // running, so this will stop the system completely.
  proc_stop_this_proc();

  while(1) { };
}

/// @brief Remove all other characters from the screen, to aid clarity.
void panic_clear_screen()
{
  const uint16_t screen_chars = screen_cols * screen_lines;
  for(uint16_t i = 0; i < screen_chars; i++)
  {
    vidmem[i * 2] = ' ';
    vidmem[(i * 2) + 1] = 0x10;
  }
}

/// @brief Print a message, starting on a specific line.
///
/// @param message The message to print. It must be less than 80 characters.
///
/// @param line The line to print on, must be less than or equal to 23. (The first line on screen is line 0)
void panic_print(const char *message, uint16_t line)
{
  uint16_t i= 0;
  uint16_t count = 0;

  KL_TRC_TRACE(TRC_LVL::FATAL, message, "\n");

  i = (line * screen_cols * 2);

  // If line is out of range, then we could scribble over some useful memory,
  // so just bail out.
  if (line >= screen_lines)
  {
    return;
  }

  // Iterate printing characters until we reach the end of the message.
  while(*message != 0)
  {
    if ((*message == '\n') || (count == screen_cols))
    {
      count = 0;
      line = line + 1;
      if(line >= screen_lines)
      {
        return;
      }
      i = line * screen_cols * 2;
      message++;
    }
    else
    {
      vidmem[i]= *message;
      message++;
      i++;
      vidmem[i]= 0x17;
      i++;
      count++;
    }
  };
};

/// @brief Provide a way for standard C & C++ library functions to panic - they generally do this where they would have
///        thrown exceptions.
extern "C" void abort()
{
  panic("Kernel abort called");
  while(1);
}
