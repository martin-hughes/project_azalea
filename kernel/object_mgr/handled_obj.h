/// @file
/// @brief Defines an object type that all objects that can be referred to by handles must inherit from.

#pragma once

class IHandledObject
{
public:
  virtual ~IHandledObject() = 0;
};
