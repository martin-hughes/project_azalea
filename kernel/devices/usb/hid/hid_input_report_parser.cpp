/// @file
/// @brief Implements a parser for HID reports.

// Known defects:
// - Limited support for arrays

//#define ENABLE_TRACING

#include "kernel_all.h"
#include "hid_input_reports.h"

namespace usb { namespace hid
{

/// @brief Parse a provided HID report given the already decoded descriptor.
///
/// @param descriptor The descriptor being used to decode this report.
///
/// @param report The raw form of the report to decode
///
/// @param report_length The number of bytes in the report.
///
/// @param[out] decode_array An array to store the decoded report in. Since the largest transfer type supported in a
///                          HID report is 4 bytes, but can be signed or unsigned, use a signed 8-byte integer to give
///                          the option for the output to be either.
///
/// @param num_output_fields The number of entries in decode_array
///
/// @return True if the report was parsed successfully, false otherwise.
bool parse_report(decoded_descriptor &descriptor,
                  void *report,
                  uint32_t report_length,
                  int64_t *decode_array,
                  uint16_t num_output_fields)
{
  bool result = true;
  uint64_t iters = 0;
  uint8_t num_bits_to_go;
  uint64_t output;
  uint8_t cur_offset;
  uint8_t *report_buffer = reinterpret_cast<uint8_t *>(report);
  uint8_t bit_offset;
  uint8_t bits_this_byte;
  uint64_t bit_mask;
  uint8_t extracted_part;

  KL_TRC_ENTRY;

  if ((report == nullptr) || (decode_array == nullptr))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Buffers cannot be nullptr");
    result = false;
  }
  else
  {
    for (report_field_description &field : descriptor.input_fields)
    {
      if (iters >= num_output_fields)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Ran out of output space\n");
        break;
      }

      num_bits_to_go = field.num_bits;
      output = 0;
      cur_offset = field.byte_offset;
      bit_offset = field.bit_offset;

      while (num_bits_to_go > 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Look at byte: ", cur_offset,
                                    ", bit: ", bit_offset,
                                    ", num to go: ", num_bits_to_go, "\n");

        // Calculate how many bits to read from the current byte.
        bits_this_byte = num_bits_to_go;
        if ((bits_this_byte + bit_offset) > 8)
        {
          bits_this_byte = 8 - bit_offset;
          KL_TRC_TRACE(TRC_LVL::FLOW, "Read ", bits_this_byte, " from this byte\n");
        }
        ASSERT(bits_this_byte <= num_bits_to_go);

        // Make space for it in the output buffer.
        output = output << bits_this_byte;

        bit_mask = (1 << bits_this_byte) - 1;
        bit_mask <<= bit_offset;
        KL_TRC_TRACE(TRC_LVL::EXTRA, "Bit mask for this byte: ", bit_mask, "\n");

        // Extract the relevant part of the report buffer byte.
        extracted_part = report_buffer[cur_offset] & static_cast<uint8_t>(bit_mask);
        extracted_part >>= bit_offset;
        output = output | extracted_part;

        // Move to the next byte.
        cur_offset++;
        num_bits_to_go -= bits_this_byte;
        bit_offset = 0;
      }

      // Decide whether this field is actually a negative value. It is if the most significant bit is set to 1 and the
      // field defines a negative logical minimum.
      if (field.logical_min < 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Check for negative and sign-extend if needed.\n");
        bit_mask = 1 << (field.num_bits - 1);

        if ((output & bit_mask) != 0)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Is needed.\n");
          bit_mask = bit_mask << 1;
          bit_mask = bit_mask - 1;
          bit_mask = ~bit_mask;
          output = output | bit_mask;
        }
      }

      KL_TRC_TRACE(TRC_LVL::FLOW, "Result for field ", iters, ": ", output, "\n");
      decode_array[iters] = output;

      ++iters;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

}; }; // Namespace usb::hid