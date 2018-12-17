/// @file
/// @brief Defines an object type that all objects that can be referred to by handles must inherit from.

#pragma once

/// @brief Classes must derive from this class if they wish to be storable in Object Manager.
///
/// That is, if they can map to a handle they must derive from this class.
class IHandledObject
{
public:
  virtual ~IHandledObject() = 0;
};
