/// @file
/// @brief Implements folders in FAT - both root and normal types.
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

#include "fat_fs.h"
#include "fat_internal.h"
#include "map_helpers.h"
#include "processor.h"
#include "types/file_wrapper.h"

#include <mutex>

namespace
{
  bool is_valid_filename_char(uint16_t ch, bool long_filename);
  ERR_CODE read_fde(uint32_t fde_index, fat::fat_dir_entry &entry, std::shared_ptr<FileWrapper> &file);
  ERR_CODE read_file_details(uint32_t &fde_index, fat::file_info &file_data, std::shared_ptr<FileWrapper> &file);
}

std::shared_ptr<fat::folder> fat::folder::create(std::shared_ptr<IBasicFile> underlying_file,
                                                 std::shared_ptr<fat_base> fs)
{
  std::shared_ptr<fat::folder> result;

  KL_TRC_ENTRY;

  result = std::shared_ptr<fat::folder>(new fat::folder(underlying_file, fs));
  result->self_weak_ptr = result;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result.get(), "\n");
  KL_TRC_EXIT;

  return result;
}

fat::folder::folder(std::shared_ptr<IBasicFile> underlying_file, std::shared_ptr<fat_base> fs) :
  underlying_file{underlying_file},
  fs{fs}
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  uint32_t cur_fde_index{0};
  file_info info;
  std::shared_ptr<FileWrapper> wrapped_file;

  KL_TRC_ENTRY;

  // For the time being, only allow this in a synchronous thread. I can't currently think of a time when this would be
  // executed asynchronously, but no doubt in future I'll change this and forget to update the constructor.
  ASSERT(!task_get_cur_thread()->is_worker_thread);
  wrapped_file = FileWrapper::create(underlying_file);

  while((result = read_file_details(cur_fde_index, info, wrapped_file)) == ERR_CODE::NO_ERROR)
  {
    kl_trc_trace(TRC_LVL::FLOW, "Found another file: ", info.canonical_name, "\n");
    std::shared_ptr<file_info> info_ptr = std::make_shared<file_info>();
    *info_ptr = info;
    filename_map.insert({info.canonical_name, info_ptr});
    canonical_names.push_back(info.canonical_name);
  }

  switch(result)
  {
  case ERR_CODE::NOT_FOUND:
  case ERR_CODE::OUT_OF_RANGE:
    KL_TRC_TRACE(TRC_LVL::FLOW, "No more files\n");
    break;

  default:
    INCOMPLETE_CODE("Haven't considered failure cases");
  }

  KL_TRC_EXIT;
}

fat::folder::~folder()
{
  // TODO: Is there anything we need to check here?
}

ERR_CODE fat::folder::get_child(const std::string &name, std::shared_ptr<IHandledObject> &child)
{
  std::string our_name_part;
  std::string child_name_part;
  ERR_CODE result{ERR_CODE::UNKNOWN};
  std::shared_ptr<file> file_obj;
  std::shared_ptr<folder> folder_obj;
  std::shared_ptr<IHandledObject> our_child;

  KL_TRC_ENTRY;

  split_name(name, our_name_part, child_name_part);

  std::unique_lock<ipc::mutex> map_guard(filename_map_mutex);
  if (map_contains(filename_map, our_name_part))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Folder contains file\n");

    std::shared_ptr<file_info> &info_block = filename_map[our_name_part];

    our_child = info_block->stored_obj.lock();

    if (!our_child)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "File not previously open - create object now.\n");

      file_obj = file::create(info_block->start_cluster, self_weak_ptr.lock(), this->fs, info_block->file_size);

      if (info_block->is_folder)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Create folder object\n");
        folder_obj = folder::create(file_obj, fs);

        info_block->stored_obj = folder_obj;
        our_child = folder_obj;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Only create file\n");
        info_block->stored_obj = file_obj;
        our_child = file_obj;
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "File returned successfully\n");
    }

    ASSERT(our_child);
    if (info_block->is_folder)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Is a folder\n");
      if (child_name_part != "")
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Request child file\n");
        result = folder_obj->get_child(child_name_part, child);
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Looking for this folder\n");
        child = folder_obj;
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Not a folder\n");
      if (child_name_part != "")
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Can't get child of a file\n");
        result = ERR_CODE::NOT_FOUND;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Child found!\n");
        child = file_obj;
        result = ERR_CODE::NO_ERROR;
      }
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "File not found\n");
    result = ERR_CODE::NOT_FOUND;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE fat::folder::add_child(const std::string &name, std::shared_ptr<IHandledObject> child)
{
  KL_TRC_ENTRY;

  INCOMPLETE_CODE("fat folder get child");

  KL_TRC_EXIT;

  return ERR_CODE::INVALID_OP;
}

ERR_CODE fat::folder::create_child(const std::string &name, std::shared_ptr<IHandledObject> &child)
{
  KL_TRC_ENTRY;

  INCOMPLETE_CODE("fat folder get child");

  KL_TRC_EXIT;

  return ERR_CODE::INVALID_OP;
}

ERR_CODE fat::folder::rename_child(const std::string &old_name, const std::string &new_name)
{
  KL_TRC_ENTRY;

  INCOMPLETE_CODE("fat folder get child");

  KL_TRC_EXIT;

  return ERR_CODE::INVALID_OP;
}

ERR_CODE fat::folder::delete_child(const std::string &name)
{
  KL_TRC_ENTRY;

  INCOMPLETE_CODE("fat folder get child");

  KL_TRC_EXIT;

  return ERR_CODE::INVALID_OP;
}

std::pair<ERR_CODE, uint64_t> fat::folder::num_children()
{
  KL_TRC_ENTRY;

  INCOMPLETE_CODE("fat folder get child");

  KL_TRC_EXIT;

  return std::pair<ERR_CODE, uint64_t>(ERR_CODE::INVALID_OP, 0);
}

std::pair<ERR_CODE, std::vector<std::string>> fat::folder::enum_children(std::string start_from, uint64_t max_count)
{
  KL_TRC_ENTRY;

  INCOMPLETE_CODE("fat folder get child");

  KL_TRC_EXIT;

  return std::pair<ERR_CODE, std::vector<std::string>>();
}

fat::fat_basic_filename_entry::operator std::string()
{
  std::string result;

  KL_TRC_ENTRY;

  // Construct a string containing the short name for this file.
  for (uint8_t i = 0; i < 11; i++)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Examine short names character: ", i, "\n");
    // Either end of the root part, or end of the extension.
    if (this->name[i] == 0x20)
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

      result += toupper(this->name[i]);
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

uint8_t fat::fat_basic_filename_entry::checksum()
{
  uint8_t cur_char;
  uint8_t checksum;
  uint8_t *v = this->name;

  KL_TRC_ENTRY;

  checksum = 0;
  for (cur_char = 11; cur_char != 0; cur_char--)
  {
    checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + *v++;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", checksum, "\n");
  KL_TRC_EXIT;
  return checksum;
}

namespace
{

/// @brief Is the provided character valid in a FAT filename?
///
/// @param ch The UTF-16 character to check.
///
/// @param long_filename Are we doing this check for a long filename (true) or short filename(false)?
///
/// @return true if the character is valid, false otherwise.
bool is_valid_filename_char(uint16_t ch, bool long_filename)
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

ERR_CODE read_file_details(uint32_t &fde_index, fat::file_info &file_data, std::shared_ptr<FileWrapper> &file)
{
  ERR_CODE result{ERR_CODE::NOT_FOUND};
  fat::fat_dir_entry fde;
  bool last_entry_was_long{false};
  uint8_t cur_lfn_checksum{0};
  bool last_was_lfn{false};
  std::string part_of_lfn;
  uint16_t cur_lfn_char;
  char next_char;
  std::string long_name;
  std::string short_name;

  KL_TRC_ENTRY;

  file_data = fat::file_info();

  while(1)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Lookup FDE index ", fde_index, "\n");
    result = read_fde(fde_index, fde, file);

    fde_index++;

    if (result != ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No more entries or another failure\n");
      break;
    }
    else if (fde.short_fn.name[0] == 0)
    {
      // no more entries...
      KL_TRC_TRACE(TRC_LVL::FLOW, "No more entries\n");
      result = ERR_CODE::NOT_FOUND;
      break;
    }
    else if (fde.short_fn.name[0] == 0xE5)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Free entry, not a terminator\n");
      file_data.long_name = "";
      last_entry_was_long = false;
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
        for (uint8_t i = 0; i < 13; i++)
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
      KL_TRC_TRACE(TRC_LVL::FLOW, "Is valid short name entry: ");
      short_name = std::string(fde.short_fn);
      KL_TRC_TRACE(TRC_LVL::FLOW, short_name, "\n");

      if ((short_name != ".") && (short_name != ".."))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Not dot-names\n");

        file_data.short_name = short_name;
        file_data.long_name = long_name;
        file_data.file_size = fde.short_fn.file_size;
        file_data.is_folder = fde.short_fn.attributes.directory;
        file_data.start_cluster = fde.short_fn.first_cluster_high;
        file_data.start_cluster <<= 16;
        file_data.start_cluster |= fde.short_fn.first_cluster_low;

        if (long_name != "")
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Check long name\n");
          if (cur_lfn_checksum == fde.short_fn.checksum())
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Adding long name: ", long_name, "\n");
            file_data.canonical_name = long_name;
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
          file_data.canonical_name = short_name;
        }
      }

      break;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE read_fde(uint32_t fde_index, fat::fat_dir_entry &entry, std::shared_ptr<FileWrapper> &file)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  uint64_t br;

  KL_TRC_ENTRY;

  result = file->read_bytes(fde_index * sizeof(fat::fat_dir_entry),
                            sizeof(fat::fat_dir_entry),
                            reinterpret_cast<uint8_t *>(&entry),
                            sizeof(fat::fat_dir_entry),
                            br);
  ASSERT((result != ERR_CODE::NO_ERROR) || (br == sizeof(fat::fat_dir_entry)));

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

}; // Local namespace
