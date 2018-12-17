/// @brief Code handling folders on a FAT filesystem.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "fat_fs.h"

fat_filesystem::fat_folder::fat_folder(fat_dir_entry file_data_record,
                                       std::shared_ptr<fat_filesystem> parent,
                                       bool root_directory) :
  parent{parent},
  is_root_dir{root_directory},
  underlying_file{file_data_record, parent, root_directory}
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

fat_filesystem::fat_folder::~fat_folder()
{

}

ERR_CODE fat_filesystem::fat_folder::get_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &child)
{
  ERR_CODE ec;
  std::shared_ptr<fat_file> file_obj;
  std::shared_ptr<fat_filesystem> parent_shared;
  std::shared_ptr<fat_folder> folder_obj;
  fat_dir_entry fde;
  kl_string our_name_part;
  kl_string child_name_part;

  KL_TRC_ENTRY;

  parent_shared = parent.lock();

  if (parent_shared)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "FS still exists\n");

    split_name(name, our_name_part, child_name_part);

    ec = this->get_dir_entry(our_name_part, fde);

    if (ec == ERR_CODE::NO_ERROR)
    {
      if (child_name_part == "")
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Requested a direct dependent\n");
        if (fde.attributes.directory)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Requested directory\n");
          folder_obj = std::make_shared<fat_folder>(fde, parent_shared, false);
          child = std::dynamic_pointer_cast<ISystemTreeLeaf>(folder_obj);
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Requested file\n");
          file_obj = std::make_shared<fat_file>(fde, parent_shared);
          child = std::dynamic_pointer_cast<ISystemTreeLeaf>(file_obj);
        }
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Requested a grandchild\n");
        if (fde.attributes.directory)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Get child of directory\n");
          folder_obj = std::make_shared<fat_folder>(fde, parent_shared, false);
          ec = folder_obj->get_child(child_name_part, child);
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Requested child of file - not possible\n");
          ec = ERR_CODE::NOT_FOUND;
        }
      }
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Parent FS destroyed\n");
    ec = ERR_CODE::DEVICE_FAILED;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", ec, "\n");
  KL_TRC_EXIT;

  return ec;
}

ERR_CODE fat_filesystem::fat_folder::add_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> child)
{
  return ERR_CODE::UNKNOWN;
}

ERR_CODE fat_filesystem::fat_folder::rename_child(const kl_string &old_name, const kl_string &new_name)
{
  return ERR_CODE::UNKNOWN;
}

ERR_CODE fat_filesystem::fat_folder::delete_child(const kl_string &name)
{
  return ERR_CODE::UNKNOWN;
}

ERR_CODE fat_filesystem::fat_folder::create_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &child)
{
  return ERR_CODE::UNKNOWN;
}


ERR_CODE fat_filesystem::fat_folder::get_dir_entry(const kl_string &name, fat_dir_entry &storage)
{
  KL_TRC_ENTRY;

  ERR_CODE ret_code = ERR_CODE::NO_ERROR;
  bool continue_looking = true;
  bool is_long_name = false;
  char short_name[12];
  fat_dir_entry long_name_entries[20];
  fat_dir_entry cur_entry;
  uint32_t cur_entry_idx = 0;
  uint8_t long_name_countup = 0;
  uint8_t long_name_cur_checksum;
  uint8_t num_long_name_entries;
  bool long_name_entry_match;

  // Validate the name as an 8.3 file name.
  if (populate_short_name(name, short_name))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Looking for short name: ", short_name, "\n");
    is_long_name = false;
  }
  else if(populate_long_name(name, long_name_entries, num_long_name_entries))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Looking for long name.\n");
    is_long_name = true;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to generate FAT filename\n");
    ret_code = ERR_CODE::NOT_FOUND;
  }

  while(continue_looking && (ret_code == ERR_CODE::NO_ERROR))
  {
    ret_code = this->read_one_dir_entry(cur_entry_idx, cur_entry);

    if (ret_code == ERR_CODE::NO_ERROR)
    {
      if (!is_long_name)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Examining: ", (const char *)cur_entry.name, "\n");
        if (kl_memcmp(cur_entry.name, short_name, 11) == 0)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Found entry\n");
          kl_memcpy(&cur_entry, &storage, sizeof(fat_dir_entry));
          continue_looking = false;
          break;
        }
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Examining entry ", long_name_countup, " of long filename\n");

        if (long_name_countup == num_long_name_entries)
        {
          if (long_name_cur_checksum == generate_short_name_checksum(cur_entry.name))
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Checksums match, found file\n");
            kl_memcpy(&cur_entry, &storage, sizeof(fat_dir_entry));
            continue_looking = false;
            break;
          }
          else
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Bad checksum\n");
            long_name_countup = 0;
          }
        }

        // This isn't an 'else' part of the if above because it is possible the mismatched checksum was actually
        // because that structure was part of the next long file name with the previous one being orphaned.
        if (long_name_countup != num_long_name_entries)
        {
          long_name_entry_match = soft_compare_lfn_entries(long_name_entries[long_name_countup], cur_entry);

          if (long_name_entry_match)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Matches so far\n");
            if (long_name_countup == 0)
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "New check, store checksum");
              long_name_cur_checksum = cur_entry.long_fn.checksum;
            }
            else
            {
              if (long_name_cur_checksum != cur_entry.long_fn.checksum)
              {
                KL_TRC_TRACE(TRC_LVL::FLOW, "Checksum mismatch\n");
                long_name_entry_match = false;
              }
            }
          }

          if (long_name_entry_match)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Still matching\n");
            long_name_countup++;
          }
          else
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Doesn't match\n");
            long_name_countup = 0;
          }
        }
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "An error occurred - 'not found' is a reasonable response.\n");
      ret_code = ERR_CODE::NOT_FOUND;
    }

    cur_entry_idx++;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Disk cluster: ", storage.first_cluster_high, " / ", storage.first_cluster_low, "\n");

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", ret_code, "\n");
  KL_TRC_EXIT;

  return ret_code;
}

/// @brief Read a single directory entry
///
/// @param entry_idx The index of the entry to read from the start of the directory
///
/// @param fde[out] Storage for the directory entry once found.
///
/// @param A suitable error code. ERR_CODE::INVALID_PARAM or ERR_CODE::STORAGE_ERROR usually mean the requested index
///        is out of range.
ERR_CODE fat_filesystem::fat_folder::read_one_dir_entry(uint32_t entry_idx, fat_dir_entry &fde)
{
  uint64_t bytes_read;
  const uint64_t buffer_size = sizeof(fat_dir_entry);
  ERR_CODE ec;

  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Looking for index ", entry_idx, "\n");
  ec = underlying_file.read_bytes(entry_idx * sizeof(fat_dir_entry),
                                  buffer_size,
                                  reinterpret_cast<uint8_t *>(&fde),
                                  buffer_size,
                                  bytes_read);

  if ((bytes_read != buffer_size) && (ec == ERR_CODE::NO_ERROR))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Truncated read\n");
    ec = ERR_CODE::STORAGE_ERROR;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", ec, "\n");
  KL_TRC_EXIT;
  return ec;
}

/// @brief If a kl_string filename is already a suitable FAT short filename, convert it into the format used on disk.
///
/// This function does not attempt to generate a short name for a long filename - it only works on filenamesthat are
/// already in 8.3 format.
///
/// @param filename The filename to convert.
///
/// @param short_name[out] The converted filename. Must be a pointer to a buffer of 12 characters (or more)
///
/// @return True if the filename could be converted, false otherwise.
bool fat_filesystem::fat_folder::populate_short_name(kl_string filename, char *short_name)
{
  bool result = true;
  kl_string first_part;
  kl_string ext_part;
  uint64_t dot_pos;

  KL_TRC_ENTRY;

  ASSERT(short_name != nullptr);

  if (filename.length() > 12)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Filename too long\n");
    result = false;
  }
  else
  {
    dot_pos = filename.find(".");
    if (dot_pos == kl_string::npos)
    {
      if (filename.length() > 8)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid 8.3. name, first part too long\n");
        result = false;
      }
      else
      {
        first_part = filename;
        ext_part = "";
      }
    }
    else if (dot_pos > 8)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid 8.3 name - dot too late");
      result = false;
    }
    else
    {
      first_part = filename.substr(0, dot_pos);
      ext_part = filename.substr(dot_pos + 1, filename.length());

      if ((first_part.length() > 8) || (ext_part.length() > 3))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid 8.3 name - parts wrong length");
        result = false;
      }
    }

    if (result)
    {
      kl_memset(short_name, 0x20, 11);
      short_name[11] = 0;

      for (int i = 0; i < first_part.length(); i++)
      {
        if(!is_valid_filename_char(first_part[i], false))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Found invalid character\n");
          result = false;
          break;
        }

        short_name[i] = first_part[i];
        if ((short_name[i] >= 'a') && (short_name[i] <= 'z'))
        {
          short_name[i] = short_name[i] - 'a' + 'A';
        }
      }

      for (int i = 0; i < ext_part.length(); i++)
      {
        if(!is_valid_filename_char(ext_part[i], false))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Found invalid character\n");
          result = false;
          break;
        }

        short_name[i + 8] = ext_part[i];
        if ((short_name[i + 8] >= 'a') && (short_name[i + 8] <= 'z'))
        {
          short_name[i + 8] = short_name[i + 8] - 'a' + 'A';
        }
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Converts a given filename into a sequence of long filename directory entries.
///
/// @param filename The filename to convert
///
/// @param long_name_entries[out] Pointer to an array of 20 fat_dir_entries to write the resulting structures in to.
///
/// @param entry_count[out] The number of long file name entry structures that are populated.
///
/// @return true if the long filename structures could be generated successfully, false otherwise.
bool fat_filesystem::fat_folder::populate_long_name(kl_string filename,
                                                    fat_dir_entry *long_name_entries,
                                                    uint8_t &num_entries)
{
  bool result = true;
  uint8_t cur_entry = 0;
  uint8_t offset;
  uint8_t remainder;

  KL_TRC_ENTRY;

  num_entries = filename.length() / 13;
  remainder = filename.length() % 13;
  if (remainder != 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Add space for ", remainder, " remainder characters\n");
    num_entries++;
  }

  cur_entry = num_entries - 1;
  long_name_entries[cur_entry].long_fn.populate();
  long_name_entries[cur_entry].long_fn.entry_idx = 1;
  long_name_entries[cur_entry].long_fn.lfn_flag = 0x0F;

  for (int i = 0; i < filename.length(); i++)
  {
    if (!is_valid_filename_char(filename[i], true))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid long filename character: ", filename[i], "\n");
      result = false;
      break;
    }

    offset = i % 13;
    if (offset < 5)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Adding char ", filename[i], " to first_chars[", offset, "]\n");
      long_name_entries[cur_entry].long_fn.first_chars[offset] = filename[i];
    }
    else if (offset < 11)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Adding char ", filename[i], " to next_chars[", offset - 5, "]\n");
      long_name_entries[cur_entry].long_fn.next_chars[offset - 5] = filename[i];
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Adding char ", filename[i], " to final_chars[", offset - 11, "]\n");
      long_name_entries[cur_entry].long_fn.final_chars[offset - 11] = filename[i];
    }

    if (((i + 1) % 13) == 0)
    {
      if (i != (filename.length() - 1))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Move to next entry\n");
        ASSERT(cur_entry != 0);
        cur_entry--;

        long_name_entries[cur_entry].long_fn.populate();
        long_name_entries[cur_entry].long_fn.entry_idx = num_entries - cur_entry;
        long_name_entries[cur_entry].long_fn.lfn_flag = 0x0F;
      }
    }
  }

  long_name_entries[0].long_fn.entry_idx |= 0x40;

  // Work out how to pad the final entry (which is, oddly enough, at index 0)
  if (remainder != 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Pad last entry - remainder = ", remainder, "\n");
    if (remainder < 5)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Set first_chars[" , remainder , "] to zero\n");
      long_name_entries[0].long_fn.first_chars[remainder] = 0;
    }
    else if (remainder < 11)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Set next_chars[", remainder - 5, "] to zero\n");
      long_name_entries[0].long_fn.next_chars[remainder - 5] = 0;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Set final_chars[", remainder - 11 , "] to zero\n");
      long_name_entries[0].long_fn.final_chars[remainder - 11] = 0;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Is the provided character valid in a FAT filename?
///
/// @param ch The UTF-16 character to check.
///
/// @param long_filename Are we doing this check for a long filename (true) or short filename(false)?
///
/// @return true if the character is valid, false otherwise.
bool fat_filesystem::fat_folder::is_valid_filename_char(uint16_t ch, bool long_filename)
{
  bool result;

  KL_TRC_ENTRY;

  result = (ch > 127) ||
           ((ch >= '0') && (ch <= '9')) ||
           ((ch >= 'a') && (ch <= 'z')) ||
           ((ch >= 'A') && (ch <= 'Z')) ||
           (ch == '$') ||
           (ch == '%') ||
           (ch == '\'') ||
           (ch == '-') ||
           (ch == '_') ||
           (ch == '@') ||
           (ch == '~') ||
           (ch == '`') ||
           (ch == '!') ||
           (ch == '(') ||
           (ch == ')') ||
           (ch == '{') ||
           (ch == '}') ||
           (ch == '^') ||
           (ch == '#') ||
           (ch == '&') ||
           (long_filename &&
            ((ch == '+') ||
             (ch == ',') ||
             (ch == ';') ||
             (ch == '=') ||
             (ch == '[') ||
             (ch == ']') ||
             (ch == ' ') ||
             (ch == '.')));

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Generate the expected checksum for a given FAT short filename
///
/// The algorithm for this is described in the Microsoft FAT specification, called 'ChkSum'
///
/// @param short_name Pointer to a character array (of at least 11 bytes) containing a short filename to checksum.
///
/// @return The generated checksum.
uint8_t fat_filesystem::fat_folder::generate_short_name_checksum(uint8_t *short_name)
{
  uint8_t cur_char;
	uint8_t checksum;

  KL_TRC_ENTRY;

  checksum = 0;
  for (cur_char = 11; cur_char != 0; cur_char--)
  {
    checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + *short_name++;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", checksum, "\n");
  KL_TRC_EXIT;
	return checksum;
}

/// @brief Compares the contents of long file name directory entries, except for fields that may not be known.
///
/// For example, the checksum field is not compared between two entries. This function will compare two short file name
/// entries as though they were long file name entries, which may not be appropriate for the caller's purposes.
///
/// @param a The first entry to compare
///
/// @param b The second entry to compare
///
/// @return True if the entries are effectively equal, false otherwise.
bool fat_filesystem::fat_folder::soft_compare_lfn_entries(const fat_dir_entry &a, const fat_dir_entry &b)
{
  bool result;

  KL_TRC_ENTRY;

  result = ((a.long_fn.entry_idx == b.long_fn.entry_idx) &&
            (a.long_fn.lfn_flag == b.long_fn.lfn_flag) &&
            (kl_memcmp(a.long_fn.first_chars, b.long_fn.first_chars, 5) == 0) &&
            (kl_memcmp(a.long_fn.next_chars, b.long_fn.next_chars, 6) == 0) &&
            (kl_memcmp(a.long_fn.final_chars, b.long_fn.final_chars, 2) == 0));

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
