/// @file
/// @brief Basic assertion functions.

#pragma once

// Basic assertions.
/// @cond
#define STRIZE_2(X) # X
#define STRIZE(X) STRIZE_2(X)
///@endcond

/// @brief Assert that a condition is true.
///
/// Panic if the assertion fails.
///
/// @param X The value to assert on.
#define ASSERT(X) if (!(X)) \
  { \
    KL_TRC_TRACE(TRC_LVL::ERROR, "Assertion failed: " # X "\nAt "  __FILE__  ":" STRIZE(__LINE__) "\n"); \
    panic ("Assertion failed: " # X "\nAt "  __FILE__  ":" STRIZE(__LINE__)); \
  }

/// @brief Marker that a particular code path hasn't been written yet.
///
/// @param X Text to display alongside the panic message.
#define INCOMPLETE_CODE(X) panic("Incomplete code: " # X)
