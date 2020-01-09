/// @file
/// @brief Some functions to help with std::map

#pragma once

/// @brief Determines whether a map contains a given key
///
/// Provides a substitute for std::map::contains() which isn't included in libcxx yet.
///
/// @tparam T Type of map to search
///
/// @tparam U Type of key within map
///
/// @param map Map to search
///
/// @param entry Key to search for in map.
///
/// @return True if the map contains entry. False otherwise.
template <typename T, typename U> bool map_contains(T &map, U &entry)
{
  bool result{false};

  auto ff = map.find(entry);
  result = (ff != map.end());

  return result;
}