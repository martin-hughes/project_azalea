#ifndef KL_STRING_H
#define KL_STRING_H

class kl_string
{
public:
  // Default constructor (empty)
  kl_string();

  // Copy constructors
  kl_string(const char *);
  kl_string(kl_string &);

  // Move constructor
  kl_string(kl_string &&);

  ~kl_string();

  // Equality operators
  bool operator ==(const kl_string &);
  bool operator ==(const char *&);

  // Inequality operators
  bool operator !=(const kl_string &);
  bool operator !=(const char *&);

  // Copy assignment operators
  kl_string &operator =(const kl_string &);
  kl_string &operator =(const char *&);

  // Move assignment operators
  kl_string &operator =(kl_string &&);

  // Other operations
  kl_string operator +(const kl_string &);
  kl_string operator +(const char *&);
  char &operator [](const unsigned long pos);

protected:
  char *string_contents;
  unsigned long buffer_length;

  void reset_string();
  void resize_buffer(unsigned long new_size);

  void move_kl_string(kl_string &);
};

#endif
