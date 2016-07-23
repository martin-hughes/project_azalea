#pragma once

// Basic assertions.
#define COMPILER_ASSERT(X) switch(1) { case 0: break; case X: break; }
#define STRIZE_2(X) # X
#define STRIZE(X) STRIZE_2(X)
#define ASSERT(X) if (!(X)) { panic ("Assertion failed: " # X "\nAt "  __FILE__  ":" STRIZE(__LINE__)); }

#define INCOMPLETE_CODE(X) panic("Incomplete code: " # X)
