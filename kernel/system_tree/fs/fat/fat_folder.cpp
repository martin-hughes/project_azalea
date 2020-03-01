/// @brief Code handling folders on a FAT filesystem.
///
// Known defects:
// - We ignore the fact that the first character of a name being 0xE5 means that the entry is free, which might allow
//   for some optimisation.
// - We ignore the translation between 0x05 and 0xE5 for Kanji.
// - No attempt is made to deal with the access/write dates or Archive flags.
// - We are picky about case, but the file system should be case-insensitive (but case-preserving). Note that the short
//   name must be stored in upper-case. The system does not, currently, allow long file name aliases for names it
//   considers would be valid short names. Also, if creating a file that seems like it might have a valid short name,
//   then that must be an uppercase name or the function will fail.
// - How does one create folders?
// - There is no thread safety!
// - num_children() and enum_children() are not tested.

//#define ENABLE_TRACING

#include <string>

#include "klib/klib.h"
#include "fat_fs.h"

#include <stdio.h>
#include <string.h>

/// @brief Standard constructor.
///
/// Call the static create method to instantiate a new folder object.
///
/// @param file_data_record The FDE of this folder.
///
/// @param fde_index The index of this folder's FDE within the parent folder.
///
/// @param fs_parent The FAT filesystem object for the whole filesystem.
///
/// @param folder_parent The parent folder of this new folder.
///
/// @param root_directory Is this actually the root directory?
fat_filesystem::fat_folder::fat_folder(fat_dir_entry file_data_record,
                                       uint32_t fde_index,
                                       std::shared_ptr<fat_filesystem> fs_parent,
                                       std::shared_ptr<fat_folder> folder_parent,
                                       bool root_directory) :
  parent{fs_parent},
  is_root_dir{root_directory},
  underlying_file{file_data_record, fde_index, folder_parent, fs_parent, root_directory}
{
  fat_dir_entry fde;
  uint32_t fde_loop_index = 0;
  ERR_CODE ec;
  std::string long_name{""};
  std::string short_name{""};
  uint8_t cur_lfn_checksum{0};
  bool last_was_lfn{false};
  std::string part_of_lfn{""};
  uint8_t i;
  uint16_t cur_lfn_char;
  char next_char;
  fat_object_details obj;

  KL_TRC_ENTRY;

  // Construct the map of names to FDE indicies.
  while (1)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Lookup FDE index ", fde_loop_index, "\n");
    ec = read_one_dir_entry(fde_loop_index, fde);

    if ((ec != ERR_CODE::NO_ERROR) || (fde.name[0] == 0))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No more entries or another failure\n");
      break;
    }

    if (fde.name[0] == 0xE5)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Free entry\n");
      long_name = "";
      last_was_lfn = false;
    }
    else if (fde.is_long_fn_entry())
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Extend current long file name\n");

      if (last_was_lfn && (cur_lfn_checksum != fde.long_fn.checksum))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Not a valid LFN continuation\n");
        last_was_lfn = false;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Extend current long file name\n");
        last_was_lfn = true;
        cur_lfn_checksum = fde.long_fn.checksum;

        part_of_lfn = "";
        for (i = 0; i < 13; i++)
        {
          cur_lfn_char = fde.long_fn.lfn_char(i);
          if (is_valid_filename_char(cur_lfn_char, true) && (cur_lfn_char < 256))
          {
            next_char = static_cast<char>(cur_lfn_char);
            part_of_lfn += next_char;
          }
        }

        long_name = part_of_lfn + long_name;
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Is valid short name entry\n");
      short_name = short_name_from_fde(fde);

      if ((short_name != ".") && (short_name != ".."))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Not dot-names\n");

        obj.short_fn = short_name;
        obj.fde = fde;
        obj.fde_index = fde_loop_index;

        KL_TRC_TRACE(TRC_LVL::FLOW, "Adding short name: ", short_name, "\n");
        short_name_to_fde_map.insert({short_name, fde_loop_index});

        if (long_name != "")
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Check long name\n");
          if (cur_lfn_checksum == generate_short_name_checksum(fde.name))
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Adding long name: ", long_name, "\n");
            long_name_to_fde_map.insert({long_name, fde_loop_index});
            obj.long_fn = long_name;
            canonical_names.insert(long_name);
          }
          else
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Discard long name due invalid checksum\n");
            long_name = "";
          }
        }

        if (long_name == "")
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "No valid long name, only short name\n");
          canonical_names.insert(short_name);
        }

        fde_to_child_map.insert({fde_loop_index, obj});
      }

      short_name = "";
      long_name = "";
      last_was_lfn = false;
    }

    fde_loop_index++;
  }

  KL_TRC_EXIT;
}

/// @brief Create a new folder object (which corresponds to opening an existing folder).
///
/// @param file_data_record The FDE of this folder.
///
/// @param fde_index The index of this folder's FDE within the parent folder.
///
/// @param fs_parent The FAT filesystem object for the whole filesystem.
///
/// @param folder_parent The parent folder of this new folder.
///
/// @param root_directory Is this actually the root directory?
///
/// @return shared_ptr to the new folder object.
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

ERR_CODE fat_filesystem::fat_folder::get_child(const std::string &name, std::shared_ptr<ISystemTreeLeaf> &child)
{
  ERR_CODE ec;
  std::shared_ptr<fat_file> file_obj;
  std::shared_ptr<fat_filesystem> parent_shared;
  std::shared_ptr<fat_folder> folder_obj;
  uint32_t found_fde_index;
  fat_dir_entry fde;
  std::string our_name_part;
  std::string child_name_part;
  std::shared_ptr<ISystemTreeLeaf> direct_child;
  bool child_already_open{false};

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
      direct_child = fde_to_child_map.find(found_fde_index)->second.child_object.lock();
      if (direct_child)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Locked child object => already opened\n");
        child_already_open = true;
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
      if (!child_already_open)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Store child object\n");
        fde_to_child_map[found_fde_index].child_object = direct_child;
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

ERR_CODE fat_filesystem::fat_folder::add_child(const std::string &name, std::shared_ptr<ISystemTreeLeaf> child)
{
  // add_child kind of represents hard-linking, and that is absolutely unsupported on a FAT filesystem.
  return ERR_CODE::INVALID_OP;
}

ERR_CODE fat_filesystem::fat_folder::rename_child(const std::string &old_name, const std::string &new_name)
{
  ERR_CODE result;
  std::shared_ptr<fat_file> file_obj;
  std::shared_ptr<fat_filesystem> parent_shared;
  uint32_t found_fde_index;
  uint32_t old_fde_index;
  fat_dir_entry fde;
  fat_dir_entry file_entries[21];
  uint8_t num_new_fdes;
  uint32_t new_fde_index;
  std::string new_short_name;
  std::string new_long_name;
  fat_object_details new_obj;

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
      result = this->get_dir_entry(old_name, fde, old_fde_index);

      if (result == ERR_CODE::NO_ERROR)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt rename\n");
        if (!populate_fdes_from_name(new_name, file_entries, num_new_fdes, true, new_long_name, new_short_name))
        {
          result = ERR_CODE::INVALID_NAME;
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Add new entries, remove old ones.\n");
          // Copy everything except the name part.
          memcpy(&file_entries[num_new_fdes - 1].attributes,
                 &fde.attributes,
                 sizeof(fat_dir_entry) - sizeof(fde.name));
          result = add_directory_entries(file_entries, num_new_fdes, new_fde_index);
          if (result == ERR_CODE::NO_ERROR)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "New directory entries added, block old ones\n");

            // Update the fde_to_child_map to remove the entry for the old index and add the entry for the new FDE
            // index.
            ASSERT(map_contains(fde_to_child_map, old_fde_index));

            if (map_contains(long_name_to_fde_map, fde_to_child_map[old_fde_index].long_fn))
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "Remove long name\n");
              long_name_to_fde_map.erase(fde_to_child_map[old_fde_index].long_fn);
              canonical_names.erase(fde_to_child_map[old_fde_index].long_fn);
            }
            else
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "Remove short name from master list\n");
              canonical_names.erase(fde_to_child_map[old_fde_index].short_fn);
            }

            short_name_to_fde_map.erase(fde_to_child_map[old_fde_index].short_fn);
            fde_to_child_map.erase(old_fde_index);

            new_obj.fde = file_entries[num_new_fdes - 1];
            new_obj.fde_index = new_fde_index;
            new_obj.long_fn = new_long_name;
            new_obj.short_fn = new_short_name;

            fde_to_child_map.insert({new_fde_index, new_obj});
            short_name_to_fde_map.insert({new_short_name, new_fde_index});
            if (new_long_name != "")
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "Insert long name as well\n");
              long_name_to_fde_map.insert({new_long_name, new_fde_index});
              canonical_names.insert(new_long_name);
            }
            else
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "Insert short name only to master list\n");
              canonical_names.insert(new_short_name);
            }


            result = unlink_fdes(old_fde_index);
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

ERR_CODE fat_filesystem::fat_folder::delete_child(const std::string &name)
{
  ERR_CODE result{ERR_CODE::UNKNOWN};
  fat_dir_entry e;
  uint32_t location;

  KL_TRC_ENTRY;

  result = this->get_dir_entry(name, e, location);
  if (result == ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found child for deletion.\n");

    if (map_contains(fde_to_child_map, location))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found child in map\n");
      if(map_contains(long_name_to_fde_map, fde_to_child_map[location].long_fn))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Erase long names\n");
        long_name_to_fde_map.erase(fde_to_child_map[location].long_fn);
        canonical_names.erase(fde_to_child_map[location].long_fn);
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Erase short names\n");
        canonical_names.erase(fde_to_child_map[location].short_fn);
      }

      short_name_to_fde_map.erase(fde_to_child_map[location].short_fn);
      fde_to_child_map.erase(location);
    }

    result = unlink_fdes(location);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE fat_filesystem::fat_folder::create_child(const std::string &name, std::shared_ptr<ISystemTreeLeaf> &child)
{
  ERR_CODE result = ERR_CODE::UNKNOWN;
  fat_dir_entry file_entries[21];
  uint8_t num_file_entries;
  uint32_t entry_idx;
  uint32_t new_idx;
  std::string new_long_name;
  std::string new_short_name;
  fat_object_details new_obj;

  KL_TRC_ENTRY;

  result = get_dir_entry(name, file_entries[0], entry_idx);

  memset(file_entries, 0, sizeof(file_entries));

  if (result == ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt to create duplicate file\n");
    result = ERR_CODE::ALREADY_EXISTS;
  }
  else if (result != ERR_CODE::NOT_FOUND)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Some other error\n");
  }
  else if(!populate_fdes_from_name(name, file_entries, num_file_entries, true, new_long_name, new_short_name))
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
    new_obj.fde_index = new_idx;
    new_obj.fde = file_entries[num_file_entries - 1];
    new_obj.long_fn = new_long_name;
    new_obj.short_fn = new_short_name;

    fde_to_child_map.insert({new_idx, new_obj});
    short_name_to_fde_map.insert({new_short_name, new_idx});
    if (new_long_name != "")
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Insert long name too\n");
      long_name_to_fde_map.insert({new_long_name, new_idx});
      canonical_names.insert(new_long_name);
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Insert short name only\n");
      canonical_names.insert(new_short_name);
    }


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
/// @return ERR_CODE::NO_ERROR or ERR_CODE::NOT_FOUND, as appropriate.
ERR_CODE fat_filesystem::fat_folder::get_dir_entry(const std::string &name,
                                                   fat_dir_entry &storage,
                                                   uint32_t &found_idx)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  bool found{false};

  KL_TRC_ENTRY;

  auto it = long_name_to_fde_map.find(name);

  if(it != long_name_to_fde_map.end())
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found in long name\n");
    found = true;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "look in short names\n");
    it = short_name_to_fde_map.find(name);

    if (it != short_name_to_fde_map.end())
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found in short names\n");
      found = true;
    }
  }

  if (found)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found an FDE index: ", it->second, "\n");
    found_idx = it->second;
    storage = fde_to_child_map[found_idx].fde;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Not found in long or short names\n");
    result = ERR_CODE::NOT_FOUND;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Disk cluster: ", storage.first_cluster_high, " / ", storage.first_cluster_low, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Read a single directory entry
///
/// @param entry_idx The index of the entry to read from the start of the directory
///
/// @param[out] fde Storage for the directory entry once found.
///
/// @return A suitable error code. ERR_CODE::INVALID_PARAM or ERR_CODE::STORAGE_ERROR usually mean the requested index
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

/// @brief If a std::string filename is already a suitable FAT short filename, convert it into the format used on disk.
///
/// This function does not attempt to generate a short name for a long filename - it only works on filenamesthat are
/// already in 8.3 format.
///
/// @param filename The filename to convert.
///
/// @param[out] short_name The converted filename. Must be a pointer to a buffer of 12 characters (or more)
///
/// @return True if the filename could be converted, false otherwise.
bool fat_filesystem::fat_folder::populate_short_name(std::string filename, char *short_name)
{
  bool result = true;
  std::string first_part;
  std::string ext_part;
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
    if (dot_pos == std::string::npos)
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
      memset(short_name, 0x20, 11);
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
/// @param[out] long_name_entries Pointer to an array of 20 fat_dir_entries to write the resulting structures in to.
///
/// @param[out] num_entries The number of long file name entry structures that are populated.
///
/// @return true if the long filename structures could be generated successfully, false otherwise.
bool fat_filesystem::fat_folder::populate_long_name(std::string filename,
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
            (memcmp(a.long_fn.first_chars, b.long_fn.first_chars, 5) == 0) &&
            (memcmp(a.long_fn.next_chars, b.long_fn.next_chars, 6) == 0) &&
            (memcmp(a.long_fn.final_chars, b.long_fn.final_chars, 2) == 0));

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
bool fat_filesystem::fat_folder::generate_basis_name_entry(std::string filename, fat_dir_entry &created_entry)
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
  std::string test_name;

  KL_TRC_ENTRY;

  memset(created_entry.name, 0x20, 11);
  converted_buffer = std::unique_ptr<char[]>(new char[filename.length() + 1]);
  memset(converted_buffer.get(), 0, filename.length() + 1);

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
  test_name = short_name_from_fde(created_entry);
  ec = get_dir_entry(test_name, found_fde, found_idx);
  requires_numeric_tail = lossy_conversion || (ec == ERR_CODE::NO_ERROR);

  while ((requires_numeric_tail) && (numeric_tail < 1000000))
  {
    add_numeric_tail(created_entry, number_main_part_chars, numeric_tail);
    test_name = short_name_from_fde(created_entry);
    ec = get_dir_entry(test_name, found_fde, found_idx);
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
/// The caller is responsible for updating the various name-fde tables.
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
      memset(&cur_entry, 0, sizeof(cur_entry));
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

  memset(number_buf, 0, 8);

  snprintf(number_buf, 8, "%d", suffix);

  num_suffix_chars = 1 + strnlen(number_buf, 8);
  ASSERT(num_suffix_chars <= 7);

  total_chars = num_suffix_chars + num_valid_chars;
  if (total_chars > 8)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Shorten main name part\n");
    num_valid_chars = 8 - num_suffix_chars;
  }

  fde.name[num_valid_chars] = '~';
  memcpy(fde.name + num_valid_chars + 1, number_buf, num_suffix_chars);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "New name: ", reinterpret_cast<const char *>(fde.name), "\n");
  KL_TRC_EXIT;
}

/// @brief Convert a given filename into a set of file directory entries, either in long or short format.
///
/// @param name_in The name to fill in to the FDEs.
///
/// @param[out] fdes An array of 21 file directory entries, at least some of which will contain the filename after this
///                  function completes.
///
/// @param[out] num_fdes_used How many of the file directory entries are populated?
///
/// @param create_basis_name Should this function generate a new basis name for 'name' and append it to the list of
///                          FDEs? This will have no effect if 'name' is already a valid short name.
///
/// @param[out] long_name_out The long name that is written to disk - if any. If no long name is written, "" is
///             returned.
///
/// @param[out] short_name_out The short name that is written to disk.
///
/// @return True if the function succeeds and the fdes and num_fdes_used are now valid. False otherwise.
bool fat_filesystem::fat_folder::populate_fdes_from_name(std::string name_in,
                                                         fat_dir_entry (&fdes)[21],
                                                         uint8_t &num_fdes_used,
                                                         bool create_basis_name,
                                                         std::string &long_name_out,
                                                         std::string &short_name_out)
{
  bool result = true;
  char short_name[12]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t checksum;

  KL_TRC_ENTRY;

  if (populate_short_name(name_in, short_name))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Creating short name: ", short_name, "\n");

    memcpy(&fdes[0].name, short_name, 11);
    fdes[0].attributes.archive = 1;

    short_name_out = short_name_from_fde(fdes[0]);
    long_name_out = "";

    num_fdes_used = 1;
  }
  else if(populate_long_name(name_in, fdes, num_fdes_used))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Creating long name.\n");
    memset(&fdes[num_fdes_used+1], 0, sizeof(fat_dir_entry));

    if (create_basis_name)
    {
      if (!generate_basis_name_entry(name_in, fdes[num_fdes_used]))
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

    long_name_out = name_in;
    short_name_out = short_name_from_fde(fdes[num_fdes_used]);
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
            KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to write long name FDE\n");
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

std::pair<ERR_CODE, uint64_t> fat_filesystem::fat_folder::num_children()
{
  uint64_t count{0};

  KL_TRC_ENTRY;

  count = fde_to_child_map.size();

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Num children: ", count, "\n");
  KL_TRC_EXIT;

  return {ERR_CODE::NO_ERROR, count};
}

std::pair<ERR_CODE, std::vector<std::string>>
fat_filesystem::fat_folder::enum_children(std::string start_from, uint64_t max_count)
{
  std::vector<std::string> child_list;
  ERR_CODE result{ERR_CODE::NO_ERROR};
  uint64_t cur_count{0};

  KL_TRC_ENTRY;

  auto it = canonical_names.begin();
  if (start_from != "")
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Use given name for start point\n");
    it = canonical_names.lower_bound(start_from);
  }

  while (((max_count == 0) || (max_count > cur_count)) && (it != canonical_names.end()))
  {
    std::string name{*it};
    child_list.push_back(std::move(name));

    cur_count++;
    it++;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Error code: ", result, ". Number of children: ", child_list.size(), "\n");
  KL_TRC_EXIT;

  return { result, std::move(child_list) };
}

/// @brief Construct a 'normal' short file name from a FAT FDE.
///
/// The FDE stores the filename in a fixed-length format that doesn't always suit us.
///
/// @param fde The FDE containing the name to normalise.
///
/// @return String containing the normalised name.
std::string fat_filesystem::fat_folder::short_name_from_fde(fat_dir_entry &fde)
{
  std::string result{""};
  KL_TRC_ENTRY;

  // Construct a string containing the short name for this file.
  for (uint8_t i = 0; i < 11; i++)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Examine short names character: ", i, "\n");
    // Either end of the root part, or end of the extension.
    if (fde.name[i] == 0x20)
    {
      if (i >= 8)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Reached end of extension\n");
        break;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Reached end of main part\n");
        i = 7;
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Normal character\n");

      if (i == 8)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Insert dot for extension\n");
        result += ".";
      }

      result += toupper(fde.name[i]);
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
