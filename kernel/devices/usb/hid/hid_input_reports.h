/// @file
/// @brief Defines a USB HID Report parser.
///
/// The parser both parses the HID descriptor, and can use that to parse the reports themselves.

#pragma once

#include <stdint.h>
#include <vector>
#include <stack>
#include <queue>

namespace usb { namespace hid {

#pragma pack (push, 1)
  /// @brief Short item header for HID report descriptor.
  ///
  /// See the HID spec for further details.
  struct hid_short_tag
  {
    /// @cond
    union
    {
      uint8_t raw; ///< Raw version. If equal to 0xFE, then not a short form header.
      struct
      {
        uint8_t size : 2; ///< Size of following item: 0, 1, 2, or 4 bytes
        uint8_t type : 2; ///< Type of following item. One of HID_TYPES.
        uint8_t tag : 4; ///< Tag for following item.
      };
    };
    /// @endcond
  };
  static_assert(sizeof(hid_short_tag) == 1, "Sizeof HID short tag wrong");
#pragma pack (pop)

  /// @brief Constants that are used in the hid_short_tag::type field.
  ///
  namespace HID_TYPES
  {
    const uint8_t MAIN = 0; ///< Main items.
    const uint8_t GLOBAL = 1; ///< Global items.
    const uint8_t LOCAL = 2; ///< Local items.
  }

  /// @brief Constants that are in the hid_short_tag::tag field if type == HID_TYPES::MAIN.
  ///
  /// The values of these constants are given in the USB HID Spec.
  namespace HID_MAIN_ITEMS
  {
    const uint8_t INPUT = 8; ///< An input item.
    const uint8_t OUTPUT = 9; ///< An output item.
    const uint8_t COLLECTION = 10; ///< Begins a new collection.
    const uint8_t FEATURE = 11; ///< A feature item.
    const uint8_t END_COLLECTION = 12; ///< Ends the most recently started collection.
  }

  /// @brief Constants that are in the hid_short_tag::tag field if type == HID_TYPES::GLOBAL.
  ///
  /// The values of these constants are given in the USB HID Spec.
  namespace HID_GLOBAL_ITEMS
  {
    const uint8_t USAGE_PAGE = 0; ///< Set the Usage page.
    const uint8_t LOGICAL_MIN = 1; ///< Set the logical minimum value for the next report field.
    const uint8_t LOGICAL_MAX = 2; ///< Set the logical maximum value for the next report field.
    const uint8_t PHYSICAL_MIN = 3; ///< Set the physical minimum value for the next report field.
    const uint8_t PHYSICAL_MAX = 4; ///< Set the physical maximum value for the next report field.
    const uint8_t UNIT_EXP = 5; ///< Set the unit exponent for the next report field.
    const uint8_t UNIT = 6; ///< Set the unit for the next report field.
    const uint8_t REPORT_SIZE = 7; ///< Tell the parser the size, in bits, of the next field in this report.
    const uint8_t REPORT_ID = 8; ///< Add a "report ID" field to the beginning of this report (NOT SUPPORTED)
    const uint8_t REPORT_COUNT = 9; ///< How many times should the next field be repeated in the decoded report.
    const uint8_t PUSH = 10; ///< Push the global parser state onto a stack.
    const uint8_t POP = 11; ///< Pop the global parser state from the stack.
  }

  /// @brief Constants that are in the hid_short_tag::tag field if type == HID_TYPES::LOCAL.
  ///
  /// The values of these constants are given in the USB HID Spec.
  namespace HID_LOCAL_ITEMS
  {
    const uint8_t USAGE = 0; ///< The Usage value of the next report (see the note in the spec about field's length)
    const uint8_t USAGE_MIN = 1; ///< For a set of sequential fields, the value to start from.
    const uint8_t USAGE_MAX = 2; ///< For a set of sequential fields, the maximum value to use.
    const uint8_t DESIGNATOR_IDX = 3; ///< The designator index of the next field.
    const uint8_t DESIGNATOR_MIN = 4; ///< For a set of sequential fields, the value to start from.
    const uint8_t DESIGNATOR_MAX = 5; ///< For a set of sequential fields, the maximum value to use.
    const uint8_t STRING_IDX = 7; ///< The string index of the next field.
    const uint8_t STRING_MIN = 8; ///< For a set of sequential fields, the value to start from.
    const uint8_t STRING_MAX = 9; ///< For a set of sequential fields, the maximum value to use.
    const uint8_t DELIMETER = 10; ///< Delimits between alternative usages (NOT SUPPORTED)
  }

  /// @brief The three types of Main item that define fields in the reports sent by the device.
  ///
  enum class HID_FIELD_TYPES
  {
    INPUT, ///< Input field.
    OUTPUT, ///< Output field.
    FEATURE, ///< Feature field.
  };

  /// @brief Global items as defined by the HID spec.
  ///
  /// These values apply to all following Main items until they are varied (or pushed)
  struct parser_global_state
  {
    uint32_t usage_page{0}; ///< The top 16 bits of Usage values.
    int32_t logical_minimum{0}; ///< The minimum value reported over the wire.
    int32_t logical_maximum{0}; ///< The maximum value reported over the wire.
    int32_t physical_minimum{0}; ///< The physical interpretation of logical_minimum.
    int32_t physical_maximum{0}; ///< The physical interpretation of logical_maximum.
    uint32_t unit_exponent{0}; ///< The power-of-ten exponent of Unit.
    uint32_t unit{0}; ///< Code defining the unit of the following fields (UNUSED)
    uint32_t report_size{0}; ///< The number of bits in the following fields.
    uint32_t report_id{0}; ///< The report ID of following fields (NOT SUPPORTED)
    uint32_t report_count{0}; ///< How many times the next Main item should be repeated.
  };

  /// @brief Field to contain details of a single usage, designator or string.
  ///
  /// These can then be stored in a queue waiting for the relevant Main item to appear.
  struct parser_local_state_field
  {
    uint32_t item{0}; ///< If the descriptor is providing a list of individual usages (etc) use this field.
    uint32_t item_min{0}; ///< If the descriptor is providing a list of mins and maximums, use this field.
    uint32_t item_max{0}; ///< If the descriptor is providing a list of mins and maximums, use this field.
    bool is_min_max{false}; ///< True if the descriptor is using max and min, false otherwise.
  };

  /// @brief The flags that can be added to input, output or feature fields.
  struct field_type_flags
  {
    union
    {
      uint16_t raw; ///< Data in raw format.
      struct
      {
        uint16_t constant : 1; ///< Data or Constant.
        uint16_t variable : 1; ///< Array or Variable.
        uint16_t relative : 1; ///< Absolute or Relative.
        uint16_t wrap : 1; ///< No wrap or wrap.
        uint16_t non_linear : 1; ///< Linear or Non-linear.
        uint16_t no_preferred_state : 1; ///< Preferred state or no preferred state.
        uint16_t null_state : 1; ///< No null position, or null position.
        uint16_t is_volatile : 1; ///< Non volatile or volatile. Reserved in Input fields.
        uint16_t is_buffered : 1; ///< Bit field or buffered bytes.
        uint16_t reserved : 7; ///< Reserved.
      }; ///< Anonymous.
    }; ///< Anonymous.
  };
  static_assert(sizeof(field_type_flags) == 2, "Size of HID field flags wrong.");

  /// @brief Store the Local items defined by the HID spec.
  ///
  /// The main items are queues because the spec effectively allows queueing of Usage, Designator and String items.
  struct parser_local_state
  {
    std::queue<parser_local_state_field> usage; ///< Stores Usages for the upcoming Main item.
    std::queue<parser_local_state_field> designator; ///< Stores Designators for the upcoming Main item.
    std::queue<parser_local_state_field> strings; ///< Stores Strings for the upcoming Main item.

    bool has_had_delimiter{false}; ///< Have we had a delimiter field yet? We don't support alternative Usages yet.
  };

  /// @brief Structure describing a single report within a collection.
  ///
  struct report_field_description
  {
    uint8_t byte_offset{0}; ///< How many complete bytes from the start of the encoded report does this field begin?
    uint8_t bit_offset{0}; ///< How many bits into that byte does this field begin?
    uint8_t num_bits{0}; ///< How many bits long is this field?
    HID_FIELD_TYPES type; ///< What type of report field are we dealing with?
    field_type_flags flags; ///< What flags are applied to this field?
    uint32_t usage{0}; ///< The usage associated with this field.
    uint32_t designator{0}; ///< The designator associated with this field.
    uint32_t string_idx{0}; ///< The string associated with this field.

    int32_t logical_min{0}; ///< Logical minimum value for this field.
    int32_t logical_max{0}; ///< Logical maximum value for this field.
    int32_t physical_min{0}; ///< Physical minimum value for this field.
    int32_t physical_max{0}; ///< Physical maximum value for this field.
    uint32_t unit_exponent{0}; ///< Unit exponent, with the same meaning as the HID spec.
    uint32_t unit{0}; ///< Unit code, with the same meaning as the HID spec.
  };

  /// @brief Structure to contain information about a single collection described by a HID device.
  ///
  struct decoded_collection
  {
    uint8_t collection_type; ///< Value given for the collection type.

    std::vector<report_field_description> report_fields; ///< Container for the reports contained in this collection.
    std::vector<decoded_collection> child_collections; ///< Container for any child collections of this report.
    uint32_t usage{0}; ///< The usage associated with this field.
    uint32_t designator{0}; ///< The designator associated with this field.
    uint32_t string_idx{0}; ///< The string associated with this field.

    /// Pointer to the parent of this collection, or nullptr if this is the root collection.
    ///
    /// This field is largely to help out the parser. It is invalid after the parsing is complete.
    decoded_collection *parent_collection{nullptr};
  };

  /// @brief Returns the fully-decoded descriptor.
  struct decoded_descriptor
  {
    decoded_collection root_collection; ///< Points to the decoded descriptor in the original tree format.

    /// Lists all decoded input fields in a way that can be easily looped over.
    ///
    std::vector<report_field_description> input_fields;

    /// Lists all decoded output fields in a way that can be easily looped over.
    ///
    std::vector<report_field_description> output_fields;

    /// Lists all decoded feature fields in a way that can be easily looped over.
    ///
    std::vector<report_field_description> feature_fields;
  };

  /// @brief Structure to hold Global and Local state for the report descriptor decoder.
  struct current_parse_state
  {
    decoded_collection root_collection; ///< The application collection defined by this device.

    decoded_collection *current_collection; ///< The collection currently having fields added to it.

    std::stack<parser_global_state> global_state_stack; ///< Global state stack.

    parser_local_state local_state; ///< Current Local states.

    /// All the input fields that have been identified in this descriptor.
    ///
    std::vector<report_field_description> all_input_fields;

    /// All the output fields that have been identified in this descriptor.
    ///
    std::vector<report_field_description> all_output_fields;

    /// All the feature fields that have been identified in this descriptor.
    ///
    std::vector<report_field_description> all_feature_fields;

    uint32_t total_input_bit_offset{0}; ///< What is the total number of bits in the input fields decoded so far?
    uint32_t total_output_bit_offset{0}; ///< What is the total number of bits in the input fields decoded so far?
    uint32_t total_feature_bit_offset{0}; ///< What is the total number of bits in the input fields decoded so far?

    /// @brief Initialise fields as needed.
    ///
    current_parse_state() : current_collection{&root_collection}
    {
      global_state_stack.emplace();
    };
  };

  bool parse_descriptor(void *raw_descriptor, uint32_t descriptor_length, decoded_descriptor &descriptor);
  bool parse_report(decoded_descriptor &descriptor,
                    void *report,
                    uint32_t report_length,
                    int64_t *decode_array,
                    uint16_t num_output_fields);

} } // namespace usb::hid