/// @file
/// @brief A simple string implementation for use in the Azalea Kernel.

#include "klib/klib.h"

namespace
{
  // This character exists so it can form a valid reference if someone attempts to use a subscript operator on a
  // kl_string but for a position that is out of bounds, so that they do not just scribble over random memory.
  char out_of_bounds;
}

kl_string::kl_string()
{
  this->buffer_length = 0;
  this->string_contents = nullptr;
}

kl_string::kl_string(const char *s)
{
  uint64_t sl = kl_strlen(s, 0) + 1;
  this->string_contents = new char [sl];
  this->buffer_length = sl;

  kl_memcpy(s, this->string_contents, sl);
}

kl_string::kl_string(const char *s, uint64_t len)
{
  uint64_t sl = kl_strlen(s, len) + 1;
  this->string_contents = new char [sl];
  this->buffer_length = sl;

  kl_memcpy(s, this->string_contents, sl - 1);
  this->string_contents[sl - 1] = 0;
}

kl_string::kl_string(const kl_string &s)
{
  this->buffer_length = s.buffer_length;
  this->string_contents = new char[this->buffer_length];
  kl_memcpy(s.string_contents, this->string_contents, this->buffer_length);
}

kl_string::kl_string(kl_string &&s)
{
  this->move_kl_string(s);
}

kl_string::~kl_string()
{
  this->reset_string();
}

const bool kl_string::operator ==(const kl_string &s) const
{
  if ((this->string_contents == nullptr) || (this->buffer_length == 0))
  {
    if ((s.string_contents == nullptr) || (s.buffer_length == 0) || (s.string_contents[0] == 0))
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  else if ((s.string_contents == nullptr) || (s.buffer_length == 0))
  {
    return (this->string_contents[0] == 0);
  }
  else
  {
    return (kl_strcmp(this->string_contents, this->buffer_length, s.string_contents, s.buffer_length) == 0);
  }
}

const bool kl_string::operator ==(const char *&s) const
{
  if (s == nullptr)
  {
    return ((this->string_contents == nullptr) || (this->buffer_length == 0) || (this->string_contents[0] == 0));
  }
  else if ((this->string_contents == nullptr) || (this->buffer_length == 0))
  {
    return (s[0] == 0);
  }
  else
  {
    return (kl_strcmp(this->string_contents, this->buffer_length, s, 0) == 0);
  }
}

const bool kl_string::operator !=(const kl_string &s) const
{
  return !(this->operator ==(s));
}
const bool kl_string::operator !=(const char *&s) const
{
  return !(this->operator == (s));
}

// Copy assignment operators
kl_string &kl_string::operator =(const kl_string &s)
{
  this->reset_string();

  this->buffer_length = s.buffer_length;
  this->string_contents = new char[this->buffer_length];

  kl_memcpy(s.string_contents, this->string_contents, this->buffer_length);

  return *this;
}

kl_string &kl_string::operator =(const char *&s)
{
  this->reset_string();

  this->buffer_length = kl_strlen(s, 0) + 1;
  this->string_contents = new char[this->buffer_length];
  kl_memcpy(s, this->string_contents, this->buffer_length);

  return *this;
}

// Move assignment operator
kl_string &kl_string::operator =(kl_string &&s)
{
  this->reset_string();
  this->move_kl_string(s);

  return *this;
}

// Other operators
kl_string kl_string::operator +(const kl_string &s) const
{
  uint64_t other_length = kl_strlen(s.string_contents, s.buffer_length);
  uint64_t our_length = kl_strlen(this->string_contents, this->buffer_length);
  uint64_t required_size = our_length + other_length + 1;
  kl_string new_string;

  new_string.buffer_length = required_size;
  new_string.string_contents = new char[required_size];

  kl_memcpy(this->string_contents, new_string.string_contents, our_length);
  kl_memcpy(s.string_contents, (new_string.string_contents + our_length), other_length + 1);

  return new_string;
}

kl_string kl_string::operator +(const char *&s) const
{
  uint64_t other_length = kl_strlen(s, 0);
  uint64_t our_length = kl_strlen(this->string_contents, this->buffer_length);
  uint64_t required_size = our_length + other_length + 1;
  kl_string new_string;

  new_string.buffer_length = required_size;
  new_string.string_contents = new char[required_size];

  kl_memcpy(this->string_contents, new_string.string_contents, our_length);
  kl_memcpy(s, (new_string.string_contents + our_length), other_length + 1);

  return new_string;
}

char &kl_string::operator [](const uint64_t pos)
{
  // It is totally acceptable to return a position that may well be beyond the limits of the string - the C++ spec says
  // that the standard library string has "undefined behaviour", so we can too.
  if ((pos > this->buffer_length) || (pos > kl_strlen(this->string_contents, this->buffer_length)))
  {
    return out_of_bounds;
  }

  return *(this->string_contents + pos);
}

const bool kl_string::operator <(const kl_string &s) const
{
  return (kl_strcmp(this->string_contents, this->buffer_length, s.string_contents, s.buffer_length) == -1);
}

const bool kl_string::operator >(const kl_string &s) const
{
  return (kl_strcmp(this->string_contents, this->buffer_length, s.string_contents, s.buffer_length) == 1);
}

const uint64_t kl_string::find(const kl_string &substr) const
{
  uint64_t substr_len = substr.length();
  if (substr_len > this->length())
  {
    return kl_string::npos;
  }

  for (uint64_t p = 0; p < (this->length() - substr.length()); p++)
  {
    if (kl_strcmp(&this->string_contents[p], substr_len, substr.string_contents, substr_len) == 0)
    {
      return p;
    }
  }

  return kl_string::npos;
}

const uint64_t kl_string::length() const
{
  return kl_strlen(this->string_contents, this->buffer_length);
}

kl_string kl_string::substr(uint64_t start, uint64_t len) const
{
  kl_string ret_string;
  uint64_t our_len = this->length();

  if ((start > our_len) || (len == 0))
  {
    ret_string = "";
  }
  else
  {
    if ((len == kl_string::npos) || (start + len > our_len))
    {
      len = our_len - start;
    }

    ret_string.buffer_length = len + 1;
    ret_string.string_contents = new char[ret_string.buffer_length];

    kl_memcpy((this->string_contents + start), ret_string.string_contents, len);
    ret_string.string_contents[len] = 0;
  }

  return ret_string;
}

void kl_string::reset_string()
{
  if (this->string_contents)
  {
    delete[] this->string_contents;
    this->string_contents = nullptr;
    this->buffer_length = 0;
  }
}

void kl_string::resize_buffer(uint64_t new_size)
{
  char *new_buf = new char[new_size];

  if ((this->buffer_length) != 0 && (this->string_contents != nullptr))
  {
    kl_memcpy(this->string_contents, new_buf, (new_size < this->buffer_length ? new_size : this->buffer_length));
    new_buf[new_size - 1] = 0;
    delete[] this->string_contents;
  }

  this->string_contents = new_buf;
  this->buffer_length = new_size;
}

void kl_string::move_kl_string(kl_string &s)
{
  this->buffer_length = s.buffer_length;
  this->string_contents = s.string_contents;

  s.buffer_length = 0;
  s.string_contents = nullptr;
}
