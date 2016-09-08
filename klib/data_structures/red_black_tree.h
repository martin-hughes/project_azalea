/// @file
/// @brief KLib Red-Black Tree implementation
///
/// kl_rb_tree provides a simple implementation of a red-black tree. It is not as capable as the standard C++ library
/// ones, but it introduces no external dependencies.
///
/// This implementation borrowed heavily from the information given on Wikipedia at
/// [https://en.wikipedia.org/wiki/Red%E2%80%93black_tree]. In particular, these rules are referred to throughout the
/// comments here.
///
/// 1. A node is coloured either red or black.
/// 2. The root is black.
/// 3. All leaves are black. In this implementation, the leaves that must be black are represented by nullptr.
/// 4. If a node is red, then both its children are black.
/// 5. Every path from a given node to any of its descendant leaf nodes contains the same number of black nodes.

#ifndef __RED_BLACK_TREE_H
#define __RED_BLACK_TREE_H

#include "klib/tracing/tracing.h"
#include "klib/panic/panic.h"
#include "klib/misc/assert.h"

/// @brief KLib Red-Black Tree
///
/// Provides a simple red-black tree implementation. Not as capable as the standard C++ library one, but with no
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
template <class key_type, class value_type> class kl_rb_tree
{
protected:

  /// @brief Data type for storing data within the tree. Represents a single node.
  template <class nkey_type, class nvalue_type> struct kl_rb_tree_node
  {
    /// @brief The key, has the usual meaning
    key_type key;

    /// @brief The value associated with the key. The tree doesn't care what this is.
    value_type value;

    /// @brief The left descendant of this node. Is nullptr if there are no descendants.
    kl_rb_tree_node<nkey_type, nvalue_type> *left;

    /// @brief The right descendant of this node. Is nullptr if there are no descendants.
    kl_rb_tree_node<nkey_type, nvalue_type> *right;

    /// @brief The parent node of this one. Is nullptr if this node is the tree root.
    kl_rb_tree_node<nkey_type, nvalue_type> *parent;

    /// @brief Is this a black node? True if black, false if red.
    bool is_black;
  };
  typedef kl_rb_tree_node<key_type, value_type> tree_node;

  /// @brief The root of this node.
  tree_node *root;

public:
  /// @brief Standard constructor. No copy or other constructor is provided at present.
  kl_rb_tree()
  {
    root = nullptr;
    left_side_last = false;
  }

  /// @brief Standard destructor.
  ///
  /// Frees all memory associated with the tree, but destroying the keys or values themselves is the responsibility of
  /// the user.
  ~kl_rb_tree()
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
    tree_node *uncle_node;
    tree_node *saved_parent;

    if (root == nullptr)
    {
      root = new tree_node;
      root->key = key;
      root->value = value;
      root->left = nullptr;
      root->right = nullptr;
      root->parent = nullptr;
      root->is_black = true;
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
            new_node->is_black = false;
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
            new_node->is_black = false;
            break;
          }
          else
          {
            search_node = search_node->right;
          }
        }
      }

      // We've just added a new red node. That could cause the tree to become unbalanced.
      // Note that in this loop, new_node could refer to the new node created above, or to a node newly made red.
      while(1)
      {
        uncle_node = find_uncle(new_node);
        if (new_node->parent == nullptr)
        {
          // The new node is at the root. Paint it black. The number of black nodes in each subtree is increased by
          // one, equally.
          new_node->is_black = true;
          ASSERT(root == new_node);
          break;
        }
        else if (new_node->parent->is_black == true)
        {
          // The child of a black node can be either colour, and red doesn't affect the length of the routes to the
          // leaves so no damage done. Nothing left to do.
          ASSERT (!new_node->is_black)
          ASSERT((new_node->left == nullptr) || (new_node->left->is_black));
          ASSERT((new_node->right == nullptr) || (new_node->right->is_black));
          break;
        }
        else if ((uncle_node != nullptr) && (uncle_node->is_black == false))
        {
          // The parent is red, so new_node can't be just yet. If the uncle is also red, then both it and the parent
          // can be coloured black. The grandparent can be switched to red at this point. However, since the
          // grandparent may be the root node (which must be black) or the child of another red node, pretend that
          // we've just inserted it and take another look through the tree to check constraints.
          new_node->parent->is_black = true;
          uncle_node->is_black = true;

          // The grandparent must exist, otherwise we couldn't have an uncle.
          new_node->parent->parent->is_black = false;

          // We've just created a new red node, so spin around to check that it still satisfies all constraints.
          new_node = new_node->parent->parent;
        }
        else
        {
          // Since our parent node is red, it cannot be root, so our grandparent node can be accessed without further
          // checking. Keep separate track of the parent node throughout, in case we move to a leaf which is nullptr.
          saved_parent = new_node->parent;
          ASSERT((new_node == saved_parent->left) || (new_node == saved_parent->right));

          if ((new_node == new_node->parent->right) && (new_node->parent == new_node->parent->parent->left))
          {
            rotate_left(new_node->parent);
            saved_parent = new_node;
            new_node = new_node->left;
          }
          else if ((new_node == new_node->parent->left) && (new_node->parent == new_node->parent->parent->right))
          {
            rotate_right(new_node->parent);
            saved_parent = new_node;
            new_node = new_node->right;
          }

          ASSERT(saved_parent->parent->is_black);
          ASSERT(saved_parent->is_black == false);
          saved_parent->is_black = true;
          saved_parent->parent->is_black = false;
          if (new_node == saved_parent->left)
          {
            rotate_right(saved_parent->parent);
          }
          else
          {
            ASSERT(new_node == saved_parent->right);
            rotate_left(saved_parent->parent);
          }

          ASSERT((new_node == nullptr) || (!new_node->is_black));
          ASSERT(saved_parent->is_black);

          break;
        }
      }

      if (!new_node->is_black)
      {
        ASSERT((new_node->left == nullptr) || (new_node->left->is_black));
        ASSERT((new_node->right == nullptr) || (new_node->right->is_black));
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

  /// @brief Verifies the tree is a valid Red-Black Tree.
  ///
  /// **THIS FUNCTION IS INTENDED FOR TEST CODE ONLY** - although it should function normally in all code.
  ///
  /// Panics if a fault is found.
  void debug_verify_tree()
  {
    debug_print_tree(root, 0);
    ASSERT(debug_check_node(root));
    debug_verify_black_length(root);
  }

protected:
  /// @brief When removing nodes, did we replace it with the left child last time?
  bool left_side_last;

  /// @brief Keep traversing down the left side of the tree from this point, looking for the leaf.
  ///
  /// @param start The node to start looking from
  ///
  /// @return The node at the leftmost leaf from the start node.
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
  /// @return The node at the rightmost leaf from the start node.
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

  /// @brief Find the uncle node for this one.
  ///
  /// The uncle is the sibling of the parent node.
  ///
  /// @param start The node to find the uncle for
  ///
  /// @return The uncle node for the starting node. If there is no uncle, for whatever reason, returns nullptr.
  tree_node *find_uncle(tree_node *start)
  {
    tree_node *result;

    ASSERT(start != nullptr);
    if ((start->parent == nullptr) || (start->parent->parent == nullptr))
    {
      // In order to have an uncle we must have a grandparent.
      result = nullptr;
    }
    else if(start->parent == start->parent->parent->left)
    {
      result = start->parent->parent->right;
    }
    else
    {
      result = start->parent->parent->left;
    }

    return result;
  }

  /// @brief Find the sibling for this node, if one exists.
  ///
  /// The sibling is the other child of this node's parent.
  ///
  /// @param start The node to find the sibling for.
  ///
  /// @return The sibling of the starting node. If there isn't one, the result is nullptr.
  tree_node *find_sibling(tree_node *start)
  {
    if ((start == nullptr) || (start->parent == nullptr))
    {
      return nullptr;
    }

    ASSERT((start->parent->left == start) || (start->parent->right == nullptr));
    if (start->parent->left == start)
    {
      return start->parent->right;
    }
    else
    {
      return start->parent->left;
    }
  }

  /// @brief Make a left tree rotation using start_node as the pivot
  ///
  /// @param start_node The top of the tree being rotated.
  void rotate_left(tree_node *start_node)
  {
    ASSERT(start_node != nullptr);
    ASSERT(start_node->right != nullptr);

    tree_node *saved_child = start_node->right;
    tree_node *saved_parent = start_node->parent;

    start_node->right = saved_child->left;
    if (start_node->right != nullptr)
    {
      start_node->right->parent = start_node;
    }
    saved_child->left = start_node;
    start_node->parent = saved_child;

    ASSERT((saved_parent == nullptr) || (start_node == saved_parent->left) || (start_node == saved_parent->right));

    if(saved_parent != nullptr)
    {
      if (saved_parent->left == start_node)
      {
        saved_parent->left = saved_child;
      }
      else
      {
        ASSERT(saved_parent->right == start_node);
        saved_parent->right = saved_child;
      }
      saved_child->parent = saved_parent;
    }
    else
    {
      saved_child->parent = nullptr;
      root = saved_child;
    }
  }

  /// @brief Make a right tree rotation using start_node as the pivot
  ///
  /// @param start_node The top of the tree being rotated.
  void rotate_right(tree_node *start_node)
  {
    ASSERT(start_node != nullptr);
    ASSERT(start_node->left != nullptr);

    tree_node *saved_child = start_node->left;
    tree_node *saved_parent = start_node->parent;

    start_node->left = saved_child->right;
    if (start_node->left != nullptr)
    {
      start_node->left->parent = start_node;
    }
    saved_child->right = start_node;
    start_node->parent = saved_child;

    ASSERT((saved_parent == nullptr) || (start_node == saved_parent->left) || (start_node == saved_parent->right));

    if(saved_parent != nullptr)
    {
      if (saved_parent->left == start_node)
      {
        saved_parent->left = saved_child;
      }
      else
      {
        saved_parent->right = saved_child;
      }
      saved_child->parent = saved_parent;
    }
    else
    {
      saved_child->parent = nullptr;
      root = saved_child;
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
    tree_node *sibling = nullptr;
    tree_node *child = nullptr;
    bool child_was_black = true;
    bool left_side_deleted = false;

    ASSERT(node != nullptr);
    KL_TRC_TRACE(TRC_LVL::FLOW, "** Removing node with key ", node->key, " from this tree:\n");
    debug_print_tree(root, 0);
    KL_TRC_TRACE(TRC_LVL::EXTRA, "======================\n");


    if ((node->left != nullptr) && (node->right != nullptr))
    {
      // Two children. We alternate between choosing a successor from the left and right sides, to try and keep the
      // tree as balanced as possible (although there is no guarantee of balanced-ness). There are no red-black
      // specific actions, as we've copied a key and value and the tree colours remain unchanged for now.
      //
      // However, removing the successor node might change that, but that will be covered by the "zero or one children"
      // case, below.
      KL_TRC_TRACE(TRC_LVL::FLOW, "Two children, ");
      if (left_side_last)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "left successor ");
        successor = find_left_leaf(node->right);
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "right successor ");
        successor = find_right_leaf(node->left);
      }

      ASSERT(successor != nullptr);
      ASSERT((successor->left == nullptr) || (successor->right == nullptr));

      KL_TRC_TRACE(TRC_LVL::FLOW, "with key ", successor->key, "\n");

      node->key = successor->key;
      node->value = successor->value;

      remove_node(successor);
    }
    else
    {
      // Zero or one children
      child = (node->left == nullptr) ? (node->right) : (node->left);

      // Replace the node with the child in the tree. The child must be black - either it was already black, or the
      // parent was, and it was red. But in the case where only one is red, we want a black survivor and can get rid
      // of the red node without affecting the tree.
      if (child != nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "One child with key ", child->key, ", coloured ", (child->is_black ? "Black" : "RED"), ", ");
        child_was_black = child->is_black;
        child->is_black = true;
        child->parent = node->parent;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "No children, ");
      }

      if (node->parent != nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "With a parent\n");
        if (node->parent->left == node)
        {
          node->parent->left = child;
          left_side_deleted = true;
        }
        else
        {
          ASSERT(node->parent->right == node);
          node->parent->right = child;
          left_side_deleted = false;
        }
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "At the root\n");
        root = child;
      }

      // If one of the two nodes was red, that's enough. But if both were black, the tree now needs rebalancing around
      // the newly promoted child node.
      if ((node->is_black) && (child_was_black) && (node->parent != nullptr))
      {
        // Use node->parent rather than child->parent (which would match the new tree structure better) because child
        // might be nullptr.
        KL_TRC_TRACE(TRC_LVL::FLOW, "Rebalancing!\n");
        rebalance_after_delete(node->parent, left_side_deleted);
      }

      delete node;
      node = nullptr;
    }
  }

  /// @brief Rebalance the RB tree after deleting a black node with a black child.
  ///
  /// @param start_node The parent of the affected part of the tree.
  ///
  /// @param left_side_deleted Was it the left hand side of start_node where the deletion occurred? This is used to get
  ///                          the direction of tree rotations correct.
  void rebalance_after_delete(tree_node *start_node, bool left_side_deleted)
  {
    ASSERT(start_node != nullptr);

    KL_TRC_TRACE(TRC_LVL::EXTRA, "Rebalancing around key ", start_node->key, " (", (left_side_deleted ? "left gone" : "right gone"), ")\n");
    KL_TRC_TRACE(TRC_LVL::EXTRA, "++++++++++++++++++++++\n");
    debug_print_tree(root, 0);
    KL_TRC_TRACE(TRC_LVL::EXTRA, "^^^^^^^^^^^^^^^^^^^^^^\n");

    tree_node *sibling = nullptr;
    bool node_was_black = false;

    if (((start_node->left == nullptr) || (start_node->left->is_black)) &&
        ((start_node->right == nullptr) || (start_node->right->is_black)))
    {
      // Both sides are black
      KL_TRC_TRACE(TRC_LVL::FLOW, "Both sides of parent are black\n");
      if ((left_side_deleted) && (start_node->right != nullptr))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Left side was deleted, sibling to the right\n");
        sibling = start_node->right;
      }
      else if (start_node->left != nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Right side was deleted, sibling to the left\n");
        sibling = start_node->left;
      }

      if ((sibling != nullptr) &&
          (((sibling->left != nullptr) && (!sibling->left->is_black)) ||
           ((sibling->right != nullptr) && (!sibling->right->is_black))))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "One red sibling-child\n");
        // At least one of the children of the deleted node's sibling is red.
        if (left_side_deleted)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Left side was deleted, rotate left\n");
          /*if (((sibling->left != nullptr) && (!sibling->left->is_black)) &&
              ((sibling->right == nullptr) || (sibling->right->is_black)))*/
          if ((sibling->left != nullptr) && (!sibling->left->is_black))
          {
            // Only the sibling's left child is red, so do an extra rotation.
            KL_TRC_TRACE(TRC_LVL::FLOW, "Right-left sibling, extra rotation\n");

            rotate_right(sibling);
            ASSERT(start_node->right->right != nullptr);
            //start_node->right->is_black = true;
            start_node->right->right->is_black = false;
          }

          rotate_left(start_node);
          if (start_node->parent->right != nullptr)
          {
            start_node->parent->right->is_black = true;
          }
          //start_node->parent->is_black = false;
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Right side deleted, rotate right\n");
          /*if (((sibling->right != nullptr) && (sibling->right->is_black == false)) &&
              ((sibling->left == nullptr) || (sibling->left->is_black)))*/
          if ((sibling->right != nullptr) && (sibling->right->is_black == false))
          {
            // Only the sibling's right child is red, so do an extra rotation.
            KL_TRC_TRACE(TRC_LVL::FLOW, "left-right sibling, extra rotation\n");

            rotate_left(sibling);
            ASSERT(start_node->left->left != nullptr);
            //start_node->left->is_black = true;
            start_node->left->left->is_black = false;
          }

          rotate_right(start_node);
          if(start_node->parent->left != nullptr)
          {
            start_node->parent->left->is_black = true;
          }
          //start_node->parent->is_black = false;
        }

        // Since we've just done a rotation, start_node can't be at the top of the tree anymore.
        ASSERT(start_node->parent != nullptr);

        if (start_node->parent->parent == nullptr)
        {
          start_node->parent->is_black = true;
        }
        if (start_node->parent->left != nullptr)
        {
          start_node->parent->left->is_black = true;
        }
        if(start_node->parent->right != nullptr)
        {
          start_node->parent->right->is_black = true;
        }
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Sibling and children all black\n");
        // The sibling and both of its children are all black.
        if (left_side_deleted)
        {
          // If this is false, then the tree wasn't originally balanced in terms of blackness.
          ASSERT(start_node->right != nullptr);
          KL_TRC_TRACE(TRC_LVL::FLOW, "Recolour right child\n");
          start_node->right->is_black = false;
        }
        else
        {
          ASSERT(start_node->left != nullptr);
          KL_TRC_TRACE(TRC_LVL::FLOW, "Recolour left child\n");
          start_node->left->is_black = false;
        }

        node_was_black = start_node->is_black;
        start_node->is_black = true;

        if (start_node->parent != nullptr)
        {
          //if (start_node->parent->is_black)
          if (node_was_black)
          {
            if (start_node->parent->left == start_node)
            {
              left_side_deleted = true;
            }
            else
            {
              left_side_deleted = false;
            }
            rebalance_after_delete(start_node->parent, left_side_deleted);
          }
          else
          {
            start_node->parent->is_black = true;
          }
        }
      }
    }
    else
    {
      // The sibling of the deleted node is red.
      KL_TRC_TRACE(TRC_LVL::FLOW, "Sibling of deleted is red\n");
      if (left_side_deleted)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Left side deleted\n");
        ASSERT((start_node->right != nullptr) && (!start_node->right->is_black));
        rotate_left(start_node);
        start_node->parent->is_black = true;
        if (start_node->parent->right != nullptr)
        {
          start_node->parent->right->is_black = true;
        }
        start_node->is_black = false;
        /*if (start_node->right != nullptr)
        {
          start_node->right->is_black = false;
        }*/
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Right side deleted\n");
        ASSERT((start_node->left != nullptr) && (!start_node->left->is_black));
        rotate_right(start_node);
        start_node->parent->is_black = true;
        if (start_node->parent->left != nullptr)
        {
          start_node->parent->left->is_black = true;
        }
        start_node->is_black = false;
        /*if (start_node->left != nullptr)
        {
          start_node->left->is_black = false;
        }*/
      }

      if (start_node->parent->left == start_node)
      {
        left_side_deleted = true;
      }
      else
      {
        left_side_deleted = false;
      }
      rebalance_after_delete(start_node, left_side_deleted);
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
  /// @return True if the tree is a valid red-black tree. False otherwise.
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

    // The root must be black
    if ((node->parent == nullptr) && (node->is_black == false))
    {
      return false;
    }

    if (node->is_black == false)
    {
      if (((node->left != nullptr) && (!node->left->is_black)) ||
          ((node->right != nullptr) && (!node->right->is_black)))
      {
        // If a node is red, it must have two black children.
        return false;
      }
    }

    return true;
  }

  /// @brief Count the number of black nodes below and including start_node.
  ///
  /// Verify that all branches have the same number of black nodes. If not, assert.
  ///
  /// @param start_node The node to start checking from
  ///
  /// @return The number of black children in any one branch, including start_node.
  unsigned long debug_verify_black_length(tree_node *start_node)
  {
    if (start_node == nullptr)
    {
      return 1;
    }
    else
    {
      unsigned long res;
      res = debug_verify_black_length(start_node->left);
      ASSERT(debug_verify_black_length(start_node->right) == res);

      if (start_node->is_black)
      {
        res = res + 1;
      }
      return res;
    }
  }

  /// @brief Dump the tree to the kernel trace.
  ///
  /// **INTENDED FOR USE IN DEBUG CODE ONLY**.
  ///
  /// @param start_node The node to dump from
  ///
  /// @param indent The indentation to use. Callers are suggested to use zero.
  void debug_print_tree(tree_node *start_node, unsigned int indent)
  {
    for (unsigned int i = 0; i < indent; i++)
    {
      KL_TRC_TRACE(TRC_LVL::EXTRA, "| ");
    }

    if (start_node == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::EXTRA, "--\n");
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::EXTRA, start_node->key, ": ", (start_node->is_black ? "B" : "R"), " - ", start_node->value, "\n");
      debug_print_tree(start_node->left, indent + 1);
      debug_print_tree(start_node->right, indent + 1);
    }
  }
};

#endif
