/// @brief Code handling folders on a FAT filesystem.
///
// Known defects:
// - We ignore the fact that the first character of a name being 0xE5 means that the entry is free, which might allow
//   for some optimisation.
// - We ignore the translation between 0x05 and 0xE5 for Kanji.
// - No attempt is made to deal with the access/write dates or Archive flags.
// - We are picky about case, but the file system should be case-insensitive (but case-preserving). Note that the short
//   name must be stored in upper-case. The system does not, currently, allow long file name aliases for names it
//   considers would be valid short names.
// - How does one create folders?
// - There is no thread safety!

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "fat_fs.h"

fat_filesystem::fat_folder::fat_folder(fat_dir_entry file_data_record,
                                       uint32_t fde_index,
                                       std::shared_ptr<fat_filesystem> fs_parent,
                                       std::shared_ptr<fat_folder> folder_parent,
                                       bool root_directory) :
  parent{fs_parent},
  is_root_dir{root_directory},
  underlying_file{file_data_record, fde_index, folder_parent, fs_parent, root_directory}
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

std::shared_ptr<fat_filesystem::fat_folder>
  fat_filesystem::fat_folder::create(fat_dir_entry file_data_record,
                                     uint32_t fde_index,
                                     std::shared_ptr<fat_filesystem> fs_parent,
                                     std::shared_ptr<fat_folder> folder_parent,
                                     bool root_directory)
{
  KL_TRC_ENTRY;

  std::shared_ptr<fat_filesystem::fat_folder> r = std::shared_ptr<fat_filesystem::fat_folder>(
    new fat_filesystem::fat_folder(file_data_record,
                                   fde_index,
                                   fs_parent,
                                   folder_parent,
                                   root_directory));

  KL_TRC_EXIT;

  return r;
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
  uint32_t found_fde_index;
  fat_dir_entry fde;
  kl_string our_name_part;
  kl_string child_name_part;
  std::shared_ptr<ISystemTreeLeaf> direct_child;

  KL_TRC_ENTRY;

  parent_shared = parent.lock();

  if (parent_shared)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "FS still exists\n");

    split_name(name, our_name_part, child_name_part);

    ec = this->get_dir_entry(our_name_part, fde, found_fde_index);

    if (ec == ERR_CODE::NO_ERROR)
    {
      // If there is already an extant file or folder object corresponding to this directory entry, retrieve it. We use
      // weak pointers to allow the system to automatically reclaim memory for closed files, but this does mean results
      // can be stale, so also remove stale results.
      if (fde_to_child_map.contains(found_fde_index))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Already got child in map\n");
        direct_child = fde_to_child_map.search(found_fde_index).lock();
        if (!direct_child)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Previous child object expired\n");
          fde_to_child_map.remove(found_fde_index);
        }
      }

      if (fde.attributes.directory)
      {
        // First, attempt to retrieve a directory object from the object we got from the cache. If we've got all the
        // caching code correct, we should be able to get a folder object from it.
        KL_TRC_TRACE(TRC_LVL::FLOW, "Requested directory\n");
        if (direct_child)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Use cached object");
          folder_obj = std::dynamic_pointer_cast<fat_folder>(direct_child);
          ASSERT(folder_obj);
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "No existing folder obj, create new.\n");
          ASSERT(!folder_obj);
          folder_obj = fat_filesystem::fat_folder::create(fde,
                                                          found_fde_index,
                                                          parent_shared,
                                                          shared_from_this(),
                                                          false);
        }

        // We may well be looking for a grandchild, but keep a hold of this folder so it can go in the cache.
        child = std::dynamic_pointer_cast<ISystemTreeLeaf>(folder_obj);
        direct_child = child;
        if (child_name_part != "")
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Looking for a grandchild!\n");
          child = nullptr;
          ec = folder_obj->get_child(child_name_part, child);
        }
      }
      else
      {
        // As above, if there was a cache result it should be a file object.
        KL_TRC_TRACE(TRC_LVL::FLOW, "Requested file\n");
        if (direct_child)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Use cached object");
          file_obj = std::dynamic_pointer_cast<fat_file>(direct_child);
          ASSERT(file_obj);
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "No existing file obj, create new.\n");
          ASSERT(!file_obj);
          file_obj = std::make_shared<fat_file>(fde, found_fde_index, shared_from_this(), parent_shared);
        }

        // There are no children of normal file objects.
        if (child_name_part == "")
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Found child file\n");
          child = std::dynamic_pointer_cast<ISystemTreeLeaf>(file_obj);
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Trying to get child of file - doesn't exist.\n");
          ec = ERR_CODE::NOT_FOUND;
        }
        direct_child = child;
      }

      // Store the child in a lookup map for use in the future.
      if (!fde_to_child_map.contains(found_fde_index))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "First time find.\n");
        fde_to_child_map.insert(found_fde_index, direct_child);
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
  // add_child kind of represents hard-linking, and that is absolutely unsupported on a FAT filesystem.
  return ERR_CODE::INVALID_OP;
}

ERR_CODE fat_filesystem::fat_folder::rename_child(const kl_string &old_name, const kl_string &new_name)
{
  ERR_CODE result;
  std::shared_ptr<fat_file> file_obj;
  std::shared_ptr<fat_filesystem> parent_shared;
  uint32_t found_fde_index;
  fat_dir_entry fde;
  fat_dir_entry file_entries[21];
  uint8_t num_new_fdes;
  uint32_t new_fde_index;
  std::weak_ptr<ISystemTreeLeaf> cache_ptr;

  KL_TRC_ENTRY;

  parent_shared = parent.lock();

  if (parent_shared)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "FS still exists\n");

    result = this->get_dir_entry(new_name, fde, found_fde_index);

    if (result == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "The new name is already in use!\n");
      result = ERR_CODE::ALREADY_EXISTS;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "New name is available\n");
      result = this->get_dir_entry(old_name, fde, found_fde_index);

      if (result == ERR_CODE::NO_ERROR)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt rename\n");
        if (!populate_fdes_from_name(new_name, file_entries, num_new_fdes, true))
        {
          result = ERR_CODE::INVALID_NAME;
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Add new entries, remove old ones.\n");
          // Copy everything except the name part.
          kl_memcpy(&fde.attributes,
                    &file_entries[num_new_fdes - 1].attributes,
                    sizeof(fat_dir_entry) - sizeof(fde.name));
          result = add_directory_entries(file_entries, num_new_fdes, new_fde_index);
          if (result == ERR_CODE::NO_ERROR)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "New directory entries added, block old ones\n");

            // Update the fde_to_child_map to remove the entry for the old index and add the entry for the new FDE
            // index.
            if (fde_to_child_map.contains(found_fde_index))
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "Found child in map\n");
              cache_ptr = fde_to_child_map.search(found_fde_index);
              fde_to_child_map.remove(found_fde_index);
              fde_to_child_map.insert(new_fde_index, cache_ptr);
            }

            result = unlink_fdes(found_fde_index);
          }
        }
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Old name not found\n");
        result = ERR_CODE::NOT_FOUND;
      }
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Parent filesystem shut down\n");
    result = ERR_CODE::STORAGE_ERROR;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE fat_filesystem::fat_folder::delete_child(const kl_string &name)
{
  ERR_CODE result{ERR_CODE::UNKNOWN};
  fat_dir_entry e;
  uint32_t location;

  KL_TRC_ENTRY;

  result = this->get_dir_entry(name, e, location);
  if (result == ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found child for deletion.\n");

    if (fde_to_child_map.contains(location))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found child in map\n");
      fde_to_child_map.remove(location);
    }

    result = unlink_fdes(location);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE fat_filesystem::fat_folder::create_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &child)
{
  ERR_CODE result = ERR_CODE::UNKNOWN;
  fat_dir_entry file_entries[21];
  uint8_t num_file_entries;
  uint32_t entry_idx;
  uint32_t new_idx;

  KL_TRC_ENTRY;

  // This isn't ideal, it causes populate_short_name and/or populate_long_name to be called twice.
  result = get_dir_entry(name, file_entries[0], entry_idx);

  kl_memset(file_entries, 0, sizeof(file_entries));

  if (result == ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt to create duplicate file\n");
    result = ERR_CODE::ALREADY_EXISTS;
  }
  else if (result != ERR_CODE::NOT_FOUND)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Some other error\n");
  }
  else if(!populate_fdes_from_name(name, file_entries, num_file_entries, true))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to generate FAT filename\n");
    result = ERR_CODE::INVALID_PARAM;
  }

  // NOT_FOUND indicates that the file doesn't already exist - which means it can be created.
  if (result == ERR_CODE::NOT_FOUND)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No errors so far, continue.\n");
    file_entries[num_file_entries - 1].attributes.archive = 1;
    result = add_directory_entries(file_entries, num_file_entries, new_idx);
  }

  if (result == ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Get child\n");
    result = get_child(name, child);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Retrieve a FAT Directory Entry structure corresponding to the given name.
///
/// @param[in] name The name of the file to search for. Can be a FAT long name. This parameter is ignored if
///                 use_raw_short_name is true.
///
/// @param[out] storage Returns the retrieved FAT Directory Entry.
///
/// @param[out] found_idx Returns the index of the returned entry in the array of structures that forms a directory in
///                       FAT.
///
/// @param[in] use_raw_short_name If this is true, the value in raw_short_name is used instead of the value in name. In
///                               this case, raw_short_name must point to a buffer of 11 characters containing a
///                               filename in the short FAT format.
///
/// @return ERR_CODE::NO_ERROR or ERR_CODE::NOT_FOUND, as appropriate.
ERR_CODE fat_filesystem::fat_folder::get_dir_entry(const kl_string &name,
                                                   fat_dir_entry &storage,
                                                   uint32_t &found_idx,
                                                   bool use_raw_short_name,
                                                   const char *raw_short_name)
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
  if (!use_raw_short_name)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Use traditional format name\n");
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
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Use raw format filename\n");
    if (raw_short_name == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No short name provided\n");
      ret_code = ERR_CODE::INVALID_PARAM;
    }
    else
    {
      is_long_name = false;
      kl_memcpy(raw_short_name, short_name, 11);
      short_name[11] = 0;
    }
  }

  while(continue_looking && (ret_code == ERR_CODE::NO_ERROR))
  {
    ret_code = this->read_one_dir_entry(cur_entry_idx, cur_entry);

    if (ret_code == ERR_CODE::NO_ERROR)
    {
      // If the first character of a FAT entry is 0x00, then there are no further entries to examine. This is a
      // deliberate optimisation in the FAT spec.
      if (cur_entry.name[0] == 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "No further entries!\n");
        continue_looking = false;
        ret_code = ERR_CODE::NOT_FOUND;
        break;
      }

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
  found_idx = cur_entry_idx;

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

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Write a FAT directory entry into the specified position in this folder's table.
///
/// @param index The position to write the FDE in to.
///
/// @param fde The FAT directory entry to write.
///
/// @return A suitable ERR_CODE.
ERR_CODE fat_filesystem::fat_folder::write_fde(uint32_t index, const fat_dir_entry &fde)
{
  ERR_CODE result = ERR_CODE::UNKNOWN;
  uint64_t br;
  uint64_t folder_size;

  KL_TRC_ENTRY;

  result = underlying_file.get_file_size(folder_size);
  if (result == ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Continue with writing FDE\n");
    if (((index + 1) * sizeof(fat_dir_entry)) > folder_size)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Expand folder\n");
      result = underlying_file.set_file_size((index + 1) * sizeof(fat_dir_entry));
    }

    if (result == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Write FDE\n");
      result = this->underlying_file.write_bytes(index * sizeof(fat_dir_entry),
                                                 sizeof(fat_dir_entry),
                                                 reinterpret_cast<const uint8_t *>(&fde),
                                                 sizeof(fat_dir_entry),
                                                 br);
    }

    if ((result == ERR_CODE::NO_ERROR) && (br != sizeof(fat_dir_entry)))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Write failed for unknown reason.\n");
      result = ERR_CODE::UNKNOWN;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Given a desired name for a new file, what "basis name" (short form) filename can we use?
///
/// There is a suggested algorithm for this task in the Microsoft FAT specification, which is roughly followed here.
/// See "The Basis-Name Generation Algorithm" for more details.
///
/// @param filename The long-form filename that the new file should have.
///
/// @param[out] created_entry A FAT directory entry containing the basis name and checksum that could be used for the
///                           new file.
///
/// @return True if a basis name could be created, false otherwise.
bool fat_filesystem::fat_folder::generate_basis_name_entry(kl_string filename, fat_dir_entry &created_entry)
{
  bool result = false;
  bool lossy_conversion = false;
  bool is_still_leading_period = true;
  uint32_t copy_position = 0;
  uint32_t last_period_position = 0;
  uint32_t i;
  std::unique_ptr<char[]> converted_buffer;
  uint32_t converted_length = 0;
  uint32_t numeric_tail = 1;
  uint8_t number_main_part_chars = 0;
  bool requires_numeric_tail;
  fat_dir_entry found_fde;
  uint32_t found_idx = 0;
  ERR_CODE ec;

  KL_TRC_ENTRY;

  kl_memset(created_entry.name, 0x20, 11);
  converted_buffer = std::unique_ptr<char[]>(new char[filename.length() + 1]);
  kl_memset(converted_buffer.get(), 0, filename.length() + 1);

  // Make uppercase and remove unsuitable characters. This will also remove all space characters.
  for (i = 0; i < filename.length(); i++)
  {
    if ((filename[i] == '.') && is_still_leading_period)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Strip leading period\n");
      lossy_conversion = true;
    }
    else
    {
      is_still_leading_period = false;

      // Strip embedded spaces.
      if (filename[i] != ' ')
      {
        if((filename[i] != '.') && !is_valid_filename_char(filename[i], false))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Character at position ", i, " (", filename[i], ") is not valid\n");
          converted_buffer[copy_position] = '_';
          lossy_conversion = true;
        }
        else if ((filename[i] >= 'a') && (filename[i] <= 'z'))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Make character at position ", i, " uppercase\n");
          converted_buffer[copy_position] = filename[i] - ('a' - 'A');
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Straight character copy\n");
          converted_buffer[copy_position] = filename[i];
        }

        if (filename[i] == '.')
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Found period\n");
          last_period_position = copy_position;
        }

        copy_position++;
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Part-converted filename: ", converted_buffer.get(), "\n");
  converted_length = copy_position;
  copy_position = 0;

  // Copy the part-converted filename into the FDE.
  i = 0;
  while ((i < converted_length) && (converted_buffer[i] != '.') && (i < 8))
  {
    created_entry.name[copy_position] = converted_buffer[i];
    i++;
    copy_position++;
    number_main_part_chars++;
  }

  i = last_period_position;
  copy_position = 8;
  while ((i < converted_length) && (copy_position < 11))
  {
    created_entry.name[copy_position] = converted_buffer[i];
    i++;
    copy_position++;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Converted filename: ", reinterpret_cast<const char *>(created_entry.name), "\n");

  // Do numeric-tail generation.
  ec = get_dir_entry(filename, found_fde, found_idx, true, reinterpret_cast<const char *>(created_entry.name));
  requires_numeric_tail = lossy_conversion || (ec == ERR_CODE::NO_ERROR);

  while ((requires_numeric_tail) && (numeric_tail < 1000000))
  {
    add_numeric_tail(created_entry, number_main_part_chars, numeric_tail);
    ec = get_dir_entry(filename, found_fde, found_idx, true, reinterpret_cast<const char *>(created_entry.name));
    if (ec == ERR_CODE::NOT_FOUND)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found a reasonable filename\n");
      requires_numeric_tail = false;
      break;
    }
    else if (ec != ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Some other error\n");
      break;
    }

    numeric_tail++;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "get_dir_entry result: ", ec, "\n");
  result = (ec == ERR_CODE::NOT_FOUND);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Add a batch of new directory entries to this directory.
///
/// @param new_fdes Pointer to an array of FAT Directory Entries
///
/// @param num_entries The number of FDEs to add. Valid range is 1-21, inclusive.
///
/// @param[out] new_idx The index of the short name FDE after it is placed.
///
/// @return A suitable error code.
ERR_CODE fat_filesystem::fat_folder::add_directory_entries(fat_dir_entry *new_fdes,
                                                           uint8_t num_entries,
                                                           uint32_t &new_idx)
{
  uint32_t found_first_idx = 0;
  uint8_t found_spaces = 0;
  ERR_CODE result = ERR_CODE::UNKNOWN;
  fat_dir_entry cur_entry;
  ERR_CODE ret_code;
  bool mid_match_group = 0;
  bool move_eod_marker = false;

  KL_TRC_ENTRY;

  for (uint32_t i = 0; i < UINT16_MAX; i++)
  {
    ret_code = this->read_one_dir_entry(i, cur_entry);

    if (ret_code == ERR_CODE::NO_ERROR)
    {
      if (cur_entry.name[0] == 0xE5)
      {
        if (mid_match_group)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Empty space continues\n");
          found_spaces++;
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Found new empty space\n");
          mid_match_group = true;
          found_spaces = 1;
          found_first_idx = i;
        }
      }
      else if(cur_entry.name[0] == 0x00)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Found end of directory\n");
        if (!mid_match_group)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Update start position\n");
          found_first_idx = i;
        }

        move_eod_marker = true;
        found_spaces = num_entries; // There's no need to continue looping, all entries after this one are empty.
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Currently in use\n");
        mid_match_group = false;
        found_spaces = 0;
      }

      if (found_spaces == num_entries)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Found sufficient space\n");
        break;
      }
    }
    else
    {
      break;
    }
  }

  if (ret_code == ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Insert FDEs starting at position: ", found_first_idx, "\n");

    for (uint8_t j = 0; j < num_entries; j++)
    {
      ret_code = write_fde(j + found_first_idx, new_fdes[j]);
      if (ret_code != ERR_CODE::NO_ERROR)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to write FDE\n");
        break;
      }
    }

    new_idx = found_first_idx + num_entries - 1;

    if (move_eod_marker)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Write empty EOD marker\n");
      kl_memset(&cur_entry, 0, sizeof(cur_entry));
      ret_code = write_fde(found_first_idx + num_entries, cur_entry);
    }
  }

  result = ret_code;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Add a numeric tail to a short filename FDE
///
/// Sufficient characters will be added or replaced to end up with a name in the format AA~XXXXX.EEE where:
///
/// - A is a character from the original name
/// - XXXXX is the number given by suffix, and can be from 1-6 characters in length, with no leading zeroes, and
/// - EEE is the original extension.
///
/// @param[out] fde Current filename in short file name format.
///
/// @param num_valid_chars The number of characters in the main part of the name that are valid - must be in the range
///                        1-8, inclusive.
///
/// @param suffix The suffix number to add as a tail.
void fat_filesystem::fat_folder::add_numeric_tail(fat_dir_entry &fde, uint8_t num_valid_chars, uint32_t suffix)
{
  KL_TRC_ENTRY;
  char number_buf[8];
  uint8_t num_suffix_chars;
  uint8_t total_chars;

  ASSERT((num_valid_chars >= 1) && (num_valid_chars <= 8));
  ASSERT(suffix < 1000000);

  kl_memset(number_buf, 0, 8);

  klib_snprintf(number_buf, 8, "%d", suffix);

  num_suffix_chars = 1 + kl_strlen(number_buf, 8);
  ASSERT(num_suffix_chars <= 7);

  total_chars = num_suffix_chars + num_valid_chars;
  if (total_chars > 8)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Shorten main name part\n");
    num_valid_chars = 8 - num_suffix_chars;
  }

  fde.name[num_valid_chars] = '~';
  kl_memcpy(number_buf, fde.name + num_valid_chars + 1, num_suffix_chars);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "New name: ", reinterpret_cast<const char *>(fde.name), "\n");
  KL_TRC_EXIT;
}

/// @brief Convert a given filename into a set of file directory entries, either in long or short format.
///
/// @param name The name to fill in to the FDEs.
///
/// @param[out] fdes An array of 21 file directory entries, at least some of which will contain the filename after this
///                  function completes.
///
/// @param[out] num_fdes_used How many of the file directory entries are populated?
///
/// @param create_basis_name Should this function generate a new basis name for 'name' and append it to the list of
///                          FDEs? This will have no effect if 'name' is already a valid short name.
///
/// @return True if the function succeeds and the fdes and num_fdes_used are now valid. False otherwise.
bool fat_filesystem::fat_folder::populate_fdes_from_name(kl_string name,
                                                         fat_dir_entry (&fdes)[21],
                                                         uint8_t &num_fdes_used,
                                                         bool create_basis_name)
{
  bool result = true;
  char short_name[12]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t checksum;

  KL_TRC_ENTRY;

  if (populate_short_name(name, short_name))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Creating short name: ", short_name, "\n");

    kl_memcpy(short_name, &fdes[0].name, 11);
    fdes[0].attributes.archive = 1;

    num_fdes_used = 1;
  }
  else if(populate_long_name(name, fdes, num_fdes_used))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Creating long name.\n");

    if (create_basis_name)
    {
      if (!generate_basis_name_entry(name, fdes[num_fdes_used]))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to generate basis name\n");
        result = false;
      }
      else
      {
        checksum = generate_short_name_checksum(fdes[num_fdes_used].name);
        for (uint8_t i = 0; i < num_fdes_used; i++)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Set checksum on entry ", i, "\n");
          fdes[i].long_fn.checksum = checksum;
        }

        num_fdes_used++;
      }
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't populate long or short names\n");
    result = false;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Mark a set of FDEs as not in use.
///
/// Starting from a short name FDE, work through it and any long name children, marking them all as free for re-use.
/// This function does not attempt to truncate a folder if the entries being freed are at the end.
///
/// @param short_name_fde_idx The index of a short file name FDE to free. This FDE and any long name children
///                           associated with it will all be freed.
///
/// @return A suitable error code.
ERR_CODE fat_filesystem::fat_folder::unlink_fdes(uint32_t short_name_fde_idx)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  fat_dir_entry fde;

  KL_TRC_ENTRY;

  result = this->read_one_dir_entry(short_name_fde_idx, fde);

  if ((result == ERR_CODE::NO_ERROR) && (fde.long_fn.lfn_flag != 0x0F))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Mark FDE at index ", short_name_fde_idx, " free\n");
    fde.name[0] = 0xE5;
    result = this->write_fde(short_name_fde_idx, fde);
    if (result == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wrote short name FDE OK\n");

      while (short_name_fde_idx > 0)
      {
        short_name_fde_idx--;
        result = this->read_one_dir_entry(short_name_fde_idx, fde);
        if ((result == ERR_CODE::NO_ERROR) && (fde.long_fn.lfn_flag == 0x0F))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Mark FDE at index ", short_name_fde_idx, " free\n");
          fde.name[0] = 0xE5;
          this->write_fde(short_name_fde_idx, fde);

          if (result != ERR_CODE::NO_ERROR)
          {
            KL_TRC_TRACE(TRC_LVL:FLOW, "Failed to write long name FDE\n");
            break;
          }
        }
        else if (result == ERR_CODE::NO_ERROR)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Reached next short name, so stop\n");
          break;
        }
      }
    }
  }
  else if (fde.long_fn.lfn_flag != 0x0F)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Tried to start from a long file name entry.\n");
    result = ERR_CODE::INVALID_OP;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
