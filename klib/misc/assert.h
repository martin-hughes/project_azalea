#pragma once

// Basic assertions.
#define COMPILER_ASSERT(X) switch(1) { case 0: break; case X: break; }
#define STRIZE_2(X) # X
#define STRIZE(X) STRIZE_2(X)
#define ASSERT(X) if (!(X)) \
  { \
    KL_TRC_TRACE((TRC_LVL_ERROR, "Assertion failed: " # X "\nAt "  __FILE__  ":" STRIZE(__LINE__) "\n")); \
    panic ("Assertion failed: " # X "\nAt "  __FILE__  ":" STRIZE(__LINE__)); \
  }

#define INCOMPLETE_CODE(X) panic("Incomplete code: " # X)
