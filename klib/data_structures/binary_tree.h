/// @file
/// @brief KLib Binary Search Tree implementation
///
/// kl_binary_tree provides a simple implementation of a binary search tree. It is not as capable as the standard C++
/// library ones, but it introduces no external dependencies.

#ifndef __BINARY_TREE_H
#define __BINARY_TREE_H

#include "klib/tracing/tracing.h"
#include "klib/panic/panic.h"
#include "klib/misc/assert.h"

/// @brief KLib Binary Search Tree
///
/// Provides a simple binary search tree implementation. Not as capable as the standard C++ library one, but with no
/// external dependencies. The tree is entirely **thread-unsafe**. Two simultaneous operations on it may leave the tree
/// in an inconsistent state.
///
/// @tparam key_type The type to be used as the key in the tree. It is assumed that the following operators are defined
///                  for key_type:
///                  - Assignment (=)
///                  - Greater than and less than (>, <)
///                  - Equality (==)
///
/// @tparam value_type The type of the values being stored in the array. This type is stored within the array using a
///                    normal assignment (=) operator, and provided back using a standard return statement. The user is
///                    responsible for ensuring this will not cause memory occupancy or other issues.
template <class key_type, class value_type> class kl_binary_tree
{
protected:

  /// @brief Data type for storing data within the tree. Represents a single node.
  template <class nkey_type, class nvalue_type> struct kl_binary_tree_node
  {
    /// @brief The key, has the usual meaning
    key_type key;

    /// @brief The value associated with the key. The tree doesn't care what this is.
    value_type value;

    /// @brief The left descendant of this node. Is nullptr if there are no descendants.
    kl_binary_tree_node<nkey_type, nvalue_type> *left;

    /// @brief The right descendant of this node. Is nullptr if there are no descendants.
    kl_binary_tree_node<nkey_type, nvalue_type> *right;

    /// @brief The parent node of this one. Is nullptr if this node is the tree root.
    kl_binary_tree_node<nkey_type, nvalue_type> *parent;
  };
  typedef kl_binary_tree_node<key_type, value_type> tree_node;

  /// @brief The root of this node.
  tree_node *root;

public:
  /// @brief Standard constructor. No copy or other constructor is provided at present.
  kl_binary_tree()
  {
    root = nullptr;
    left_side_last = false;
  }

  /// @brief Standard destructor.
  ///
  /// Frees all memory associated with the tree, but destroying the keys or values themselves is the responsibility of
  /// the user.
  ~kl_binary_tree()
  {
    if (root != nullptr)
    {
      delete_node(root);
      root = nullptr;
    }
  }

  /// @brief Insert a key-value pair in to the tree
  ///
  /// @param key The key to use. Must provide the necessary operators.
  ///
  /// @param value The value to insert. This is opaque to the tree data structure.
  void insert(key_type key, value_type value)
  {
    tree_node *search_node;
    tree_node *new_node;

    if (root == nullptr)
    {
      root = new tree_node;
      root->key = key;
      root->value = value;
      root->left = nullptr;
      root->right = nullptr;
      root->parent = nullptr;
    }
    else
    {
      search_node = root;

      while (1)
      {
        if (key == search_node->key)
        {
          search_node->value = value;
          break;
        }
        else if (key < search_node->key)
        {
          if (search_node->left == nullptr)
          {
            search_node->left = new tree_node;
            new_node = search_node->left;
            new_node->key = key;
            new_node->value = value;
            new_node->left = nullptr;
            new_node->right = nullptr;
            new_node->parent = search_node;
            break;
          }
          else
          {
            search_node = search_node->left;
          }
        }
        else // key > search_node->key
        {
          if (search_node->right == nullptr)
          {
            search_node->right = new tree_node;
            new_node = search_node->right;
            new_node->key = key;
            new_node->value = value;
            new_node->left = nullptr;
            new_node->right = nullptr;
            new_node->parent = search_node;
            break;
          }
          else
          {
            search_node = search_node->right;
          }
        }
      }
    }
  }

  /// @brief Remove the node associated with key from the tree
  ///
  /// @param key The key to remove. This has no effect on the lifetime of either key or the associated value, which is
  ///            controlled by the user. The key **must** be contained within the tree.
  void remove(key_type key)
  {
    tree_node *node_to_delete = node_search(nullptr, key);
    ASSERT((node_to_delete != nullptr) && (node_to_delete->key == key));

    remove_node(node_to_delete);
  }

  /// @brief Determines if key is in the tree
  ///
  /// @param key The key to look for
  ///
  /// @return True if the key is found in the tree, False otherwise.
  bool contains(key_type key)
  {
    tree_node *result = node_search(nullptr, key);
    return ((result != nullptr) && (result->key == key));
  }

  /// @brief Return the value associated with the key
  ///
  /// @param key The key to look for. Key **must** be part of the tree.
  ///
  /// @return The value associated with the key.
  value_type search(key_type key)
  {
    tree_node *result = node_search(nullptr, key);
    ASSERT((result != nullptr) && (result->key == key));

    return result->value;
  }

  /// @brief Verifies the tree is a valid Binary Search Tree.
  ///
  /// **THIS FUNCTION IS INTENDED FOR TEST CODE ONLY** - although it should function normally in all code.
  ///
  /// Panics if a fault is found.
  void debug_verify_tree()
  {
    ASSERT(debug_check_node(root));
  }

protected:
  /// @brief When removing nodes, did we replace it with the left child last time?
  bool left_side_last;

  /// @brief Keep traversing down the left side of the tree from this point, looking for the leaf.
  ///
  /// @param start The node to start looking from
  ///
  /// @retun The node at the leftmost leaf from the start node.
  tree_node *find_left_leaf(tree_node *start)
  {
    if (start->left == nullptr)
    {
      return start;
    }
    else
    {
      return find_left_leaf(start->left);
    }
  }

  /// @brief Keep traversing down the right side of the tree from this point, looking for the leaf.
  ///
  /// @param start The node to start looking from
  ///
  /// @retun The node at the rightmost leaf from the start node.
  tree_node *find_right_leaf(tree_node *start)
  {
    if (start->right == nullptr)
    {
      return start;
    }
    else
    {
      return find_right_leaf(start->right);
    }
  }

  /// @brief Removes the specified node from the tree
  ///
  /// After removing the node, join up the tree in the most appropriate manner
  ///
  /// @param node The node to remove.
  void remove_node(tree_node *node)
  {
    tree_node *successor = nullptr;
    tree_node *parent;

    if ((node->left != nullptr) && (node->right != nullptr))
    {
      // Two children. We alternate between choosing a successor from the left and right sides, to try and keep the
      // tree as balanced as possible (although there is no guarantee of balanced-ness)
      if (left_side_last)
      {
        successor = find_left_leaf(node->right);
      }
      else
      {
        successor = find_right_leaf(node->left);
      }

      ASSERT(successor != nullptr);
      ASSERT((successor->left == nullptr) || (successor->right == nullptr));

      node->key = successor->key;
      node->value = successor->value;

      remove_node(successor);
    }
    else
    {
      // Zero or one children
      parent = node->parent;
      successor = node->left;
      if (node->right != nullptr)
      {
        successor = node->right;
      }

      if (parent == nullptr)
      {
        root = successor;
      }
      else
      {
        // Parent != nullptr
        if (parent->left == node)
        {
          parent->left = successor;
        }
        else
        {
          ASSERT(parent->right == node);
          parent->right = successor;
        }
      }

      if (successor != nullptr)
      {
        successor->parent = parent;
      }

      delete node;
    }
  }

  /// @brief Delete a node and all its descendants.
  ///
  /// @param node The node to delete
  void delete_node(tree_node *node)
  {
    if (node->left != nullptr)
    {
      delete_node(node->left);
    }
    if (node->right != nullptr)
    {
      delete_node(node->right);
    }
    delete node;
  }

  /// @brief Search for a node in the tree below start_node
  ///
  /// @param start_node The node to start looking from
  ///
  /// @param key The key to look for
  ///
  /// @return The node with the correct key, or the closest match if there is no exact match.
  tree_node *node_search(tree_node *start_node, key_type key)
  {
    if (start_node == nullptr)
    {
      start_node = root;
    }

    if (start_node->key == key)
    {
      return start_node;
    }
    else if (key < start_node->key)
    {
      if (start_node->left != nullptr)
      {
        return node_search(start_node->left, key);
      }
      else
      {
        return start_node;
      }
    }
    else // key > start_node->key
    {
      if (start_node->right != nullptr)
      {
        return node_search(start_node->right, key);
      }
      else
      {
        return start_node;
      }
    }
  }

  /// @brief Check the tree below node for consistency.
  ///
  /// **This code is intended for use by test code only.**
  ///
  /// @param node The node to start checking from
  ///
  /// @return True if the tree is a valid binary search tree. False otherwise.
  bool debug_check_node(tree_node *node)
  {
    tree_node *search_node;
    key_type search_val;

    if (node == nullptr)
    {
      return true;
    }

    if ((node->left == nullptr) || (node->right == nullptr))
    {
      search_val = node->key;
      search_node = node;
      while(search_node->parent != nullptr)
      {
        if ((search_node != search_node->parent->left) && (search_node != search_node->parent->right))
        {
          return false;
        }
        if (((search_node->parent->left == node) && (search_val > search_node->parent->key)) ||
           ((search_node->parent->right == node) && (search_val < search_node->parent->key)))
        {
          return false;
        }
        search_node = search_node->parent;
      }
    }

    if (((node->left != nullptr) && (!debug_check_node(node->left))) ||
        ((node->right != nullptr) && (!debug_check_node(node->right))))
    {
      return false;
    }

    return true;
  }
};

#endif
