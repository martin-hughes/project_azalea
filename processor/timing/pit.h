#ifndef __X64_PROC_PIT
#define __X64_PROC_PIT

// Define the interface for controlling the processors timing capabilities.
// At the moment, this is only to start the PIT with a simple repetitive loop.

// TODO: At present, the PIT is not used. Remove?

extern "C" void asm_proc_init_pit();

#endif
