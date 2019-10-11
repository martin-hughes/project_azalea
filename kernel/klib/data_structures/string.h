#ifndef KL_STRING_H
#define KL_STRING_H

#include <stdint.h>

/// @brief A simplified version of the standard C++ string class
class kl_string
{
public:
  /// A value to indicate that a given character was not found, or that a range should extend the maximum possible
  /// length. No string could fill all of memory (otherwise there'd be no room for code!) so ~0 is acceptable.
  const static uint64_t npos = ~((uint64_t)0);

  /// @brief Default constructor. Creates an empty string.
  kl_string();

  ////////////////////////////////
  // Constructors / Destructors //
  ////////////////////////////////

  /// @brief Constructor that copies the provided string
  ///
  /// @param s The string to copy in to this one.
  kl_string(const char *s);

  /// @brief Constructor that copies the provided string
  ///
  /// @param s The string to copy in to this one.
  kl_string(const kl_string &s);

  /// @brief Move constructor
  ///
  /// Moves the provided string in to this one and blanks it.
  ///
  /// @param s The string to move to this one.
  kl_string(kl_string &&s);

  /// @brief Constructor that copies the provided string, but with a limited length.
  ///
  /// @param s The string to copy in to this one.
  ///
  /// @param len The maximum length of string to copy. If the string terminates in less than len bytes, the string will
  ///            only be copied up to the terminator.
  kl_string(const char *s, uint64_t len);

  /// @brief Standard destructor
  ~kl_string();

  ////////////////////////
  // Equality operators //
  ////////////////////////

  /// @brief Standard equality operator
  ///
  /// @param s String to compare against
  ///
  /// @return True if equal.
  const bool operator ==(const kl_string &s) const;

  /// @brief Standard equality operator
  ///
  /// @param s String to compare against
  ///
  /// @return True if equal.
  const bool operator ==(const char *&s) const;

  // Inequality operators

  /// @brief Standard inequality operator
  ///
  /// @param s String to compare against
  ///
  /// @return True if not equal.
  const bool operator !=(const kl_string &s) const;

  /// @brief Standard inequality operator
  ///
  /// @param s String to compare against
  ///
  /// @return True if not equal.
  const bool operator !=(const char *&s) const;

  //////////////////////////
  // Assignment operators //
  //////////////////////////

  // Copy assignment operators

  /// @brief Standard copy assignment operator
  ///
  /// @param s Copy the string s into this one.
  ///
  /// @return reference to this string.
  kl_string &operator =(const kl_string &s);

  /// @brief Standard copy assignment operator
  ///
  /// @param s Copy the string s into this one.
  ///
  /// @return reference to this string.
  kl_string &operator =(const char *&s);

  // Move assignment operators

  /// @brief Standard move assignment operator
  ///
  /// @param s Move the string s into this one.
  ///
  /// @return A reference to this string.
  kl_string &operator =(kl_string &&s);

  /////////////////////
  // Other operators //
  /////////////////////

  /// @brief Append string to this one.
  ///
  /// @param s String to append.
  ///
  /// @return Copy of this string.
  kl_string operator +(const kl_string &s) const;

  /// @brief Append string to this one.
  ///
  /// @param s String to append.
  ///
  /// @return Copy of this string.
  kl_string operator +(const char *&s) const;

  /// @brief Reference a character within this string
  ///
  /// @param pos The character to reference.
  ///
  /// @return Reference to the selected character.
  char &operator [](const uint64_t pos);

  /// @brief Standard less-than operator.
  ///
  /// @param s String to compare with
  ///
  /// @return true if this string comes before s alphabetically.
  const bool operator <(const kl_string &s) const;

  /// @brief Standard greater-than operator.
  ///
  /// @param s String to compare with
  ///
  /// @return true if this string comes after s alphabetically.
  const bool operator >(const kl_string &s) const;

  ///////////////////////
  // String operations //
  ///////////////////////

  /// @brief Find the first instance of the provided string within this one.
  ///
  /// @param substr The string to find within this one.
  ///
  /// @return The position of substr within this one, starting from 0. If substr is not in this one, kl_string::npos is
  ///         returned.
  const uint64_t find(const kl_string &substr) const;

  /// @brief Find the last instance of the provided string within this one.
  ///
  /// @param substr The string to find within this one.
  ///
  /// @return The position of the last instance of substr within this string, indexed from the beginning = 0. If substr
  ///         is not found, kl_string::npos is returned.
  const uint64_t find_last(const kl_string &substr) const;

  /// @brief Return the character length of this string
  ///
  /// @return The number of characters in this string, not including the trailing 0.
  const uint64_t length() const;

  /// @brief Returns a section of this string
  ///
  /// @param start The zero-based position that the substring should start from
  ///
  /// @param len The length of the substring. Use kl_string::npos to return the remainder of the string.
  ///
  /// @return A string containing the requested substring.
  kl_string substr(uint64_t start, uint64_t len) const;

protected:
  /// A buffer containing the string stored here. May be larger than is required.
  char *string_contents;

  /// The current length of the buffer used in string_contents.
  unsigned long buffer_length;

  /// Destroys string_contents and buffer_length. Effectively sets the string to "".
  void reset_string();

  /// Resize the buffer in string_contents to a new size.
  ///
  /// @param new_size How big, in bytes, the string's buffer will become.
  void resize_buffer(uint64_t new_size);

  /// Move the string provided in to this one, and destroy it.
  ///
  /// @param s The string to move in to this one.
  void move_kl_string(kl_string &s);
};

#endif
