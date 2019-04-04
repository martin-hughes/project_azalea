/// @file
/// @brief Implements a kernel-stopping panic function.

#include "panic.h"
#include "processor/processor.h"

#include <stdlib.h>

void panic_print(const char *message, uint16_t line);
void panic_clear_screen();

char *vidmem = (char *)0xFFFFFFFF000b8000;
const uint16_t screen_lines = 25;
const uint16_t screen_cols = 80;

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

// As expected, print a kernel panic message on to the screen, and stop running.
[[noreturn]] void panic(const char *message)
{
	// Stop interrupts reaching this processor, in order to prevent most race
	// conditions.
	proc_stop_interrupts();

	// Print a simple message on the screen.
	panic_clear_screen();
	panic_print("KERNEL PANIC", 0);
	panic_print("------------", 1);
	panic_print(message, 3);

	// Stop all other processors too. It's possible that a another processor
	// could panic at the same time as this one, but we'll live with that race.
	proc_stop_other_procs();

	// Finally, we don't want to continue. This processor should be the last one
	// running, so this will stop the system completely.
	proc_stop_this_proc();

	while(1) { };
}

// Remove all other characters from the screen, for clarity.
void panic_clear_screen()
{
	const uint16_t screen_chars = screen_cols * screen_lines;
	for(uint16_t i = 0; i < screen_chars; i++)
	{
		vidmem[i * 2] = ' ';
		vidmem[(i * 2) + 1] = 0x10;
	}
}

// Print a message, starting on a specific line. message must be less than 80
// characters, and line must be less than or equal to 23. (The first line is
// line 0)
void panic_print(const char *message, uint16_t line)
{
	uint16_t i= 0;
	uint16_t count = 0;

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