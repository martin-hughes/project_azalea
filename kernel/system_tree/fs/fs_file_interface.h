/// @file
/// @brief Declare interfaces common to file-like objects.

#pragma once

#include "user_interfaces/error_codes.h"

/// @brief Interface for all objects that support arbitrarily sized reads.
class IReadable
{
public:
  virtual ~IReadable() { };

  /// @brief Read bytes from a readable object
  ///
  /// Reads a contiguous set of bytes from the object into the buffer given by `buffer`.
  ///
  /// @param start The first byte in the object to read from.
  ///
  /// @param length The number of bytes to be read.
  ///
  /// @param[out] buffer A buffer to store the read data in to. It should be large enough, or the read will be
  ///                    truncated! If the function fails, the buffer may still have been modified by the function
  ///                    call.
  ///
  /// @param buffer_length The length of the buffer. The buffer can be larger or smaller than the number of bytes to be
  ///                      read, but if smaller then the maximum number of bytes to be read will be that which fits in
  ///                      buffer
  ///
  /// @param[out] bytes_read The number of bytes that were actually read. This may not be as many as requested. Reasons
  ///                        include, but are not limited to the end of file being reached, or a buffer that is too
  ///                        small.
  ///
  /// @return An appropriate choice from `ERR_CODE`. If the read succeeds, even if the number of bytes is not as
  ///         requested, then the result will be `NO_ERROR`.
  virtual ERR_CODE read_bytes(uint64_t start,
                              uint64_t length,
                              uint8_t *buffer,
                              uint64_t buffer_length,
                              uint64_t &bytes_read) = 0;
};

/// @brief Interface for objects that support arbitrarily sized writes
class IWritable
{
public:
  virtual ~IWritable() { };

  /// @brief Write bytes to a writable object
  ///
  /// Writes a contiguous set of bytes into the object from the buffer given by `buffer`.
  ///
  /// @param start The first byte in the object to write to.
  ///
  /// @param length The number of bytes to be written.
  ///
  /// @param buffer A buffer to write the data from. It should be large enough, or the write will be truncated! If
  ///        the write is truncated, the object may still have been modified.
  ///
  /// @param buffer_length The length of the buffer. The buffer can be larger or smaller than the number of bytes to be
  ///                      written, but if smaller then the maximum number of bytes to be written will be the same as
  ///                      the length of the buffer.
  ///
  /// @param[out] bytes_written The number of bytes that were actually written. This may not be as many as requested.
  ///                           Reasons include, but are not limited to the file system becoming full, or the buffer
  ///                           not being long enough.
  ///
  /// @return An appropriate choice from `ERR_CODE`. If the write succeeds, even if the number of bytes is not as
  ///         requested, then the result will be `NO_ERROR`.
  virtual ERR_CODE write_bytes(uint64_t start,
                               uint64_t length,
                               const uint8_t *buffer,
                               uint64_t buffer_length,
                               uint64_t &bytes_written) = 0;
};

/// @brief Interface for objects that act like files on a traditional file system.
class IBasicFile: public IReadable, public IWritable
{
public:
  virtual ~IBasicFile() { };

  /// @brief Return the length of the complete file.
  ///
  /// This is the number of bytes in the file if it were fully read in to memory. It is not necessarily the same number
  /// of bytes the file occupies on disk - in future, the system may support sparse files or native compression for
  /// example.
  ///
  /// @param[out] file_size Provided that the return value is NO_ERROR, the length of the file, in bytes. If an error
  ///                       occurs the result is unspecified. The value of `file_size` may have changed from before the
  ///                       function was called.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE get_file_size(uint64_t &file_size) = 0;

  /// @brief Set the length of the file.
  ///
  /// After the operation completes, this will be the number of bytes in the file if it were fully read into memory -
  /// it is not necessarily the same number of bytes the file actually occupies in its storage medium.
  ///
  /// This function can be used to both truncate and extend files. If a file is extended, it is padded with zeroes.
  ///
  /// @param file_size The number of bytes to set the size of the file to.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE set_file_size(uint64_t file_size) = 0;
};
