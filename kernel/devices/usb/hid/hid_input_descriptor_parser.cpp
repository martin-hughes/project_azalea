/// @file
/// @brief Implements parsers for HID Report Descriptors.
///
/// Despite the filename, this parser isn't limited to Input reports.
//
// Known defects:
// - Alternative Usages, Designator and String fields are not supported.
// - Report ID fields aren't treated properly. To prevent bad behaviour we simply say the parsing failed if a report ID
//   is encountered.

//#define ENABLE_TRACING

#include "hid_input_reports.h"

#include "klib/klib.h"

using namespace usb::hid;

namespace
{
  bool handle_item(current_parse_state &parser, hid_short_tag tag, uint32_t item_data, uint8_t data_length);
  bool handle_main_item(current_parse_state &parser, hid_short_tag tag, uint32_t item_data, uint8_t data_length);
  bool handle_global_item(current_parse_state &parser, hid_short_tag tag, uint32_t item_data, uint8_t data_length);
  bool handle_local_item(current_parse_state &parser, hid_short_tag tag, uint32_t item_data, uint8_t data_length);
  bool add_new_field(current_parse_state &parser, HID_FIELD_TYPES type, uint32_t item_data);
  uint32_t get_next_field_index_val(std::queue<parser_local_state_field> &field_queue);
}

namespace usb::hid
{

/// @brief Convert the packed, coded, HID descriptor into a more easily usable form.
///
/// @param raw_descriptor Pointer to the buffer containing the raw-format descriptor.
///
/// @param descriptor_length The number of bytes in the raw descriptor.
///
/// @param[out] descriptor The decoded form of the descriptor.
///
/// @return True if the descriptor was successfully parsed, false otherwise.
bool parse_descriptor(std::unique_ptr<uint8_t[]> &raw_descriptor,
                      uint32_t descriptor_length,
                      decoded_descriptor &descriptor)
{
  bool result = true;

  KL_TRC_ENTRY;

  uint8_t *raw_ptr = raw_descriptor.get();
  uint16_t offset = 0;
  hid_short_tag *tag;
  uint8_t data_length;
  uint32_t data_buffer;
  uint32_t data_mask;
  current_parse_state parser;

  while (offset < descriptor_length)
  {
    tag = reinterpret_cast<hid_short_tag *>(raw_ptr);

    if (tag->raw == 0xFE)
    {
      INCOMPLETE_CODE("Long tag found!\n");
      break;
    }

    KL_TRC_TRACE(TRC_LVL::FLOW, "Type: ", tag->type, ", Tag: ", tag->tag, " - ");

    switch(tag->size)
    {
    case 3:
      data_length = 4;
      break;

    case 2:
    case 1:
    case 0:
      data_length = tag->size;
      break;

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown tag size\n");
      data_length = 0;
      break;
    }

    raw_ptr++;
    data_buffer = *(reinterpret_cast<uint32_t *>(raw_ptr));

    // Set the unused parts of the data buffer to zero. This allows the parser to look at bits of a item even if the
    // encoded format hasn't included those bits on the wire.
    data_mask = ~0;
    data_mask = data_mask << (data_length * 8);
    data_mask = ~data_mask;
    data_buffer = data_buffer & data_mask;

    KL_TRC_TRACE(TRC_LVL::FLOW, "Tag: ", tag->raw, ", Data: ", data_buffer, ", length: ", data_length, "\n");

    // Advance the pointers for the next pass.
    raw_ptr += data_length;
    offset += (1 + data_length);

    if(!handle_item(parser, *tag, data_buffer, data_length))
    {
      result = false;
    }
  }

  descriptor.input_fields.swap(parser.all_input_fields);
  descriptor.output_fields.swap(parser.all_output_fields);
  descriptor.feature_fields.swap(parser.all_feature_fields);
  descriptor.root_collection = parser.root_collection;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result:", result, "\n");
  KL_TRC_EXIT;

  return result;
}

} // Namespace usb::hid

namespace
{
  /// @brief Handle parsing of a single descriptor item
  ///
  /// @param[inout] parser Object containing the current state of the parser.
  ///
  /// @param tag Short format tag for the item to parse.
  ///
  /// @param item_data Storage for up to 4 bytes of data associated with this item.
  ///
  /// @param data_length How many of the least-significant bytes of item_data are valid (max 4)
  ///
  /// @return True if the item was parsed successfully. False otherwise.
  bool handle_item(current_parse_state &parser, hid_short_tag tag, uint32_t item_data, uint8_t data_length)
  {
    bool result = true;

    KL_TRC_ENTRY;

    ASSERT(data_length <= 4);
    ASSERT(parser.current_collection != nullptr);

    switch(tag.type)
    {
    case HID_TYPES::MAIN:
      result = handle_main_item(parser, tag, item_data, data_length);
      break;

    case HID_TYPES::GLOBAL:
      result = handle_global_item(parser, tag, item_data, data_length);
      break;

    case HID_TYPES::LOCAL:
      result = handle_local_item(parser, tag, item_data, data_length);
      break;

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown item type (3)\n");
      result = false;
      break;
    }

    KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
    KL_TRC_EXIT;

    return result;
  }

  /// @brief Handle parsing of a Main descriptor item
  ///
  /// @param[inout] parser Object containing the current state of the parser.
  ///
  /// @param tag Short format tag for the item to parse. Must be for a Main item.
  ///
  /// @param item_data Storage for up to 4 bytes of data associated with this item.
  ///
  /// @param data_length How many of the least-significant bytes of item_data are valid (max 4)
  ///
  /// @return True if the item was parsed successfully. False otherwise.
  bool handle_main_item(current_parse_state &parser, hid_short_tag tag, uint32_t item_data, uint8_t data_length)
  {
    bool result = true;
    decoded_collection new_collection;
    decoded_collection *collection_ptr;

    KL_TRC_ENTRY;

    ASSERT(tag.type == HID_TYPES::MAIN);

    switch(tag.tag)
    {
    case HID_MAIN_ITEMS::INPUT:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Input item\n");
      result = add_new_field(parser, HID_FIELD_TYPES::INPUT, item_data);
      break;

    case HID_MAIN_ITEMS::OUTPUT:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Output item\n");
      result = add_new_field(parser, HID_FIELD_TYPES::OUTPUT, item_data);
      break;

    case HID_MAIN_ITEMS::COLLECTION:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Collection\n");
      new_collection.parent_collection = parser.current_collection;
      new_collection.collection_type = item_data;
      new_collection.usage = get_next_field_index_val(parser.local_state.usage);
      new_collection.designator = get_next_field_index_val(parser.local_state.designator);
      new_collection.string_idx = get_next_field_index_val(parser.local_state.strings);
      parser.current_collection->child_collections.push_back(new_collection);
      parser.current_collection = &parser.current_collection->child_collections.back();
      break;

    case HID_MAIN_ITEMS::FEATURE:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Feature\n");
      result = add_new_field(parser, HID_FIELD_TYPES::FEATURE, item_data);
      break;

    case HID_MAIN_ITEMS::END_COLLECTION:
      KL_TRC_TRACE(TRC_LVL::FLOW, "End collection\n");
      ASSERT(parser.current_collection);
      collection_ptr = parser.current_collection->parent_collection;
      if (collection_ptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Null out parent collection pointer\n");
        // Ensure that this isn't used after the end of parsing. This is really a cross check that the parser isn't
        // crazy.
        collection_ptr->parent_collection = nullptr;
      }
      parser.current_collection = collection_ptr;
      break;

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown Main item tag: ", tag.tag, "\n");
      result = false;
      break;
    }

    // Reset the parser's local state after each Main item.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Reset parser state\n");
    parser.local_state = parser_local_state();

    KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
    KL_TRC_EXIT;

    return result;
  }

  /// @brief Handle parsing of a Global descriptor item
  ///
  /// @param[inout] parser Object containing the current state of the parser.
  ///
  /// @param tag Short format tag for the item to parse. Must be for a Global item.
  ///
  /// @param item_data Storage for up to 4 bytes of data associated with this item.
  ///
  /// @param data_length How many of the least-significant bytes of item_data are valid (max 4)
  ///
  /// @return True if the item was parsed successfully. False otherwise.
  bool handle_global_item(current_parse_state &parser, hid_short_tag tag, uint32_t item_data, uint8_t data_length)
  {
    bool result = true;
    int32_t signed_data = static_cast<int32_t>(item_data);

    KL_TRC_ENTRY;

    ASSERT(tag.type == HID_TYPES::GLOBAL);

    // If the top most bit of item_data is set, then it represents a negative value - however, the field can vary in
    // length, so we need to check 1 and two byte fields.
    if ((data_length == 1) && ((item_data & 0x80) != 0))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Bit extend 1-byte value\n");
      signed_data = static_cast<int32_t>(0xFFFFFF00UL | item_data);
    }
    else if ((data_length == 2) && ((item_data & 0x8000) != 0))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Bit extend 2-byte value\n");
      signed_data = static_cast<int32_t>(0xFFFF0000UL | item_data);
    }

    switch(tag.tag)
    {
    case HID_GLOBAL_ITEMS::USAGE_PAGE:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Usage page: ", item_data, "\n");
      parser.global_state_stack.top().usage_page = item_data;
      break;

    case HID_GLOBAL_ITEMS::LOGICAL_MIN:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Logical minimum: ", signed_data, "\n");
      parser.global_state_stack.top().logical_minimum = signed_data;
      break;

    case HID_GLOBAL_ITEMS::LOGICAL_MAX:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Logical maximum: ", signed_data, "\n");
      parser.global_state_stack.top().logical_maximum = signed_data;
      break;

    case HID_GLOBAL_ITEMS::PHYSICAL_MIN:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Physical minimum: ", signed_data, "\n");
      parser.global_state_stack.top().physical_minimum = signed_data;
      break;

    case HID_GLOBAL_ITEMS::PHYSICAL_MAX:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Physical maximum: ", signed_data, "\n");
      parser.global_state_stack.top().physical_maximum = signed_data;
      break;

    case HID_GLOBAL_ITEMS::UNIT_EXP:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unit exponent: ", item_data, "\n");
      parser.global_state_stack.top().unit_exponent = item_data;
      break;

    case HID_GLOBAL_ITEMS::UNIT:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unit: ", item_data, "\n");
      parser.global_state_stack.top().unit = item_data;
      break;

    case HID_GLOBAL_ITEMS::REPORT_SIZE:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Report size: ", item_data, "\n");
      parser.global_state_stack.top().report_size = item_data;
      break;

    case HID_GLOBAL_ITEMS::REPORT_ID:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Report ID: ", item_data, "\n");
      parser.global_state_stack.top().report_id = item_data;
      result = false;
      break;

    case HID_GLOBAL_ITEMS::REPORT_COUNT:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Report count: ", item_data, "\n");
      parser.global_state_stack.top().report_count = item_data;
      break;

    case HID_GLOBAL_ITEMS::PUSH:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Push\n");
      parser.global_state_stack.emplace();
      break;

    case HID_GLOBAL_ITEMS::POP:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Pop\n");
      parser.global_state_stack.pop();
      if (parser.global_state_stack.empty())
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Global state stack underflow\n");
        result = false;
      }
      break;

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown global item tag: ", tag.tag, "\n");
      result = false;
      break;
    }

    KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
    KL_TRC_EXIT;

    return result;
  }

  /// @brief Handle parsing of a Local descriptor item
  ///
  /// @param[inout] parser Object containing the current state of the parser.
  ///
  /// @param tag Short format tag for the item to parse. Must be for a Local item.
  ///
  /// @param item_data Storage for up to 4 bytes of data associated with this item.
  ///
  /// @param data_length How many of the least-significant bytes of item_data are valid (max 4)
  ///
  /// @return True if the item was parsed successfully. False otherwise.
  bool handle_local_item(current_parse_state &parser, hid_short_tag tag, uint32_t item_data, uint8_t data_length)
  {
    bool result = true;
    parser_local_state_field field;

    KL_TRC_ENTRY;

    ASSERT(tag.type == HID_TYPES::LOCAL);

    if (parser.local_state.has_had_delimiter)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Ignore state after a delimiter\n");
    }
    else
    {
      // If the usage field is only one or two bytes it is to be treated as the bottom 16-bits of a 32-bit field, where
      // the top 16 bits are the current usage page.
      if (((tag.tag == HID_LOCAL_ITEMS::USAGE) ||
           (tag.tag == HID_LOCAL_ITEMS::USAGE_MIN) ||
           (tag.tag == HID_LOCAL_ITEMS::USAGE_MAX)) &&
          (data_length < 4))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Update usage field\n");
        item_data = item_data | (parser.global_state_stack.top().usage_page << 16);
      }

      switch(tag.tag)
      {
      case HID_LOCAL_ITEMS::USAGE:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Usage: ", item_data, "\n");
        field.item = item_data;
        parser.local_state.usage.push(field);
        break;

      case HID_LOCAL_ITEMS::USAGE_MIN:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Usage minimum: ", item_data, "\n");
        if (parser.local_state.usage.empty() || !parser.local_state.usage.back().is_min_max)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Add min-max to back of usage queue\n");
          field.is_min_max = true;
          parser.local_state.usage.push(field);
        }
        parser.local_state.usage.back().item = item_data;
        parser.local_state.usage.back().item_min = item_data;
        break;

      case HID_LOCAL_ITEMS::USAGE_MAX:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Usage maximum: ", item_data, "\n");
        if (parser.local_state.usage.empty() || !parser.local_state.usage.back().is_min_max)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Add min-max to back of usage queue\n");
          field.is_min_max = true;
          parser.local_state.usage.push(field);
        }
        parser.local_state.usage.back().item_max = item_data;
        break;

      case HID_LOCAL_ITEMS::DESIGNATOR_IDX:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Designator index: ", item_data, "\n");
        field.item = item_data;
        parser.local_state.designator.push(field);
        break;

      case HID_LOCAL_ITEMS::DESIGNATOR_MIN:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Designator minimum: ", item_data, "\n");
        if (parser.local_state.designator.empty() || !parser.local_state.designator.back().is_min_max)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Add min-max to back of designator queue\n");
          field.is_min_max = true;
          parser.local_state.designator.push(field);
        }
        parser.local_state.designator.back().item = item_data;
        parser.local_state.designator.back().item_min = item_data;
        break;

      case HID_LOCAL_ITEMS::DESIGNATOR_MAX:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Designator maximum: ", item_data, "\n");
        if (parser.local_state.designator.empty() || !parser.local_state.designator.back().is_min_max)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Add min-max to back of designator queue\n");
          field.is_min_max = true;
          parser.local_state.designator.push(field);
        }
        parser.local_state.designator.back().item_max = item_data;
        break;

      case HID_LOCAL_ITEMS::STRING_IDX:
        KL_TRC_TRACE(TRC_LVL::FLOW, "String index: ", item_data, "\n");
        field.item = item_data;
        parser.local_state.strings.push(field);
        break;

      case HID_LOCAL_ITEMS::STRING_MIN:
        KL_TRC_TRACE(TRC_LVL::FLOW, "String minimum: ", item_data, "\n");
        if (parser.local_state.strings.empty() || !parser.local_state.strings.back().is_min_max)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Add min-max to back of strings queue\n");
          field.is_min_max = true;
          parser.local_state.strings.push(field);
        }
        parser.local_state.strings.back().item = item_data;
        parser.local_state.strings.back().item_min = item_data;
        break;

      case HID_LOCAL_ITEMS::STRING_MAX:
        KL_TRC_TRACE(TRC_LVL::FLOW, "String maximum: ", item_data, "\n");
        if (parser.local_state.strings.empty() || !parser.local_state.strings.back().is_min_max)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Add min-max to back of strings queue\n");
          field.is_min_max = true;
          parser.local_state.strings.push(field);
        }
        parser.local_state.strings.back().item_max = item_data;
        break;

      case HID_LOCAL_ITEMS::DELIMETER:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Delimeter: ", item_data, "\n");
        parser.local_state.has_had_delimiter = true;
        break;

      default:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown local item tag: ", tag.tag, "\n");
        result = false;
        break;
      }
    }

    KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
    KL_TRC_EXIT;

    return result;
  }

  /// @brief Given the current state of the parser, add new fields to the decoded descriptor.
  ///
  /// @param parser The current state of the parser to use.
  ///
  /// @param type Is this an input, output, or feature field?
  ///
  /// @param item_data The data associated with the encoded form of this field.
  ///
  /// @return True if the field was added successfully, false otherwise.
  bool add_new_field(current_parse_state &parser, HID_FIELD_TYPES type, uint32_t item_data)
  {
    bool result = true;
    report_field_description new_field;

    KL_TRC_ENTRY;

    new_field.flags.raw = static_cast<uint16_t>(item_data);
    new_field.num_bits = static_cast<uint8_t>(parser.global_state_stack.top().report_size);
    new_field.type = type;

    KL_TRC_TRACE(TRC_LVL::FLOW, "Adding ", parser.global_state_stack.top().report_count, "fields\n");
    for (uint32_t i = 0; i < parser.global_state_stack.top().report_count; i++)
    {
      // Calculate the bit and byte offsets.

      // Get the next queued values for these fields.
      if (new_field.flags.constant)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Constant input, skip using up a usage field.\n");
      }
      else if (new_field.flags.variable)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Variable input\n");
        new_field.usage = get_next_field_index_val(parser.local_state.usage);
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Array input - use min.\n");
        ASSERT(parser.local_state.usage.size() > 0);
        new_field.usage = parser.local_state.usage.front().item_min;
      }

      new_field.designator = get_next_field_index_val(parser.local_state.designator);
      new_field.string_idx = get_next_field_index_val(parser.local_state.strings);

      // Copy in the global state for this field.
      new_field.logical_min = parser.global_state_stack.top().logical_minimum;
      new_field.logical_max = parser.global_state_stack.top().logical_maximum;
      new_field.physical_min = parser.global_state_stack.top().physical_minimum;
      new_field.physical_max = parser.global_state_stack.top().physical_minimum;
      new_field.unit_exponent = parser.global_state_stack.top().unit_exponent;
      new_field.unit = parser.global_state_stack.top().unit;

      // Calculate the bit and byte offsets, and add this field to the collection, as well as the vector of all known
      // fields.
      switch(type)
      {
      case HID_FIELD_TYPES::INPUT:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Add new input field\n");
        new_field.bit_offset = parser.total_input_bit_offset % 8;
        new_field.byte_offset = parser.total_input_bit_offset / 8;
        parser.total_input_bit_offset += new_field.num_bits;
        parser.all_input_fields.push_back(new_field);
        break;

      case HID_FIELD_TYPES::OUTPUT:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Add new output field\n");
        new_field.bit_offset = parser.total_output_bit_offset % 8;
        new_field.byte_offset = parser.total_output_bit_offset / 8;
        parser.total_output_bit_offset += new_field.num_bits;
        parser.all_output_fields.push_back(new_field);
        break;

      case HID_FIELD_TYPES::FEATURE:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Add new feature field\n");
        new_field.bit_offset = parser.total_feature_bit_offset % 8;
        new_field.byte_offset = parser.total_feature_bit_offset / 8;
        parser.total_feature_bit_offset += new_field.num_bits;
        parser.all_feature_fields.push_back(new_field);
        break;
      }
      parser.current_collection->report_fields.push_back(new_field);

    }

    KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
    KL_TRC_EXIT;

    return result;
  }

  /// @brief Given the queue of {Usage, Designator, String} indicies, grab the next one.
  ///
  /// @param field The relevant queue to get a value from.
  ///
  /// @return The next value extracted from the queue.
  uint32_t get_next_field_index_val(std::queue<parser_local_state_field> &field_queue)
  {
    uint32_t result = 0;
    bool should_pop_front = true;

    KL_TRC_ENTRY;

    if (!field_queue.empty())
    {
      parser_local_state_field &field = field_queue.front();
      should_pop_front = false;
      KL_TRC_TRACE(TRC_LVL::FLOW, "Queue not empty\n");
      if (field.is_min_max)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Update using min-max\n");
        result = field.item;
        field.item++;
        if (field.item > field.item_max)
        {
          if (field_queue.size() > 1)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Used all, pop.\n");
            should_pop_front = true;
          }
          else
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "No more usage values, keep using this one.\n");
            field.item--;
          }
        }
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Update using static value\n");
        result = field.item;
        should_pop_front = true;
      }

      // Should we remove the front of the queue?
      if (should_pop_front && field_queue.size() > 1)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Removing front of queue.\n");
        field_queue.pop();
      }
    }

    KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
    KL_TRC_EXIT;

    return result;
  }
} // Local namespace
