# System Tree - Overview

## Contents

- [Introduction](#introduction)
- [Implementation Overview](#implementation-overview)
- [Differences](#differences)
- [Restrictions](#restrictions)

## Introduction

The kernel's System Tree is roughly analogous to Linux's Virtual Filesystem. It forms a tree containing objects that
are accessed by a name formed of a "directory part" and a "filename". However, like the VFS, it is capable of referring
to objects other than files, but unlike the VFS not every object stored in the tree is capable of being dealt with like
a file.

For example, here are some examples of things that can be accessed via the System Tree*:

- Files
- Devices
- Kernel Objects
- Virtual files (like the Linux /proc system)

(* Not all of these are in the initial version of System Tree. Largely because the system as a whole doesn't have much
capability yet.)

As you'd expect, it is possible to mount additions to the System Tree to extend its functionality. All of these
additions have their code in subdirectories of system_tree/.

Eventually System Tree will be secured using access control lists, but that is a project for the future.

## Implementation Overview

The System Tree is implemented by classes that expose two interfaces (yes I know , C++ doesn't have interfaces - take
it to mean they inherit abstract classes):

- ISystemTreeBranch - which roughly represents a directory in "normal" filesystem terms.
- IHandledObject - which roughly represents a file in "normal" filesystem terms.

There are no restrictions on how classes implement these interfaces. This allows one instance of a class to be
responsible for (for example) an entire filesystem on a disk - the choice is up to the developer. More details on these
interfaces can be found in the documentation in this folder.

The root of the tree is given the name "\" - like the root directory "/" in Linux, and is implemented by the class
system_tree_root, which implements ISystemTreeBranch.

Developers only need to implement the interfaces to participate in the System Tree, but default classes are provided
that, if inherited from, should simplify the task of the developer. Likewise, a variety of helper functions are
provided. It is strongly suggested to use these, to avoid inconsistencies in behaviour between different parts of the
tree.

**Details of the default classes and helper functions still to come**

## Differences

- Unlike Linux, System Tree uses the \ character for its path separator (hopefully allowing / to be used in normal
  filenames)
- Unlike Windows, System Tree is case sensitive throughout (Windows does support a case sensitive file system, but they
  choose to make it case insensitive in behaviour)

## Restrictions

In its current iteration, there are some very boring restrictions that apply:

- Only ASCII characters in the range 32 - 122 (inclusive) are permitted, except for \ (character 92) which is a path
  separator.
 - Needless to say, this set of permitted characters might change without warning!
- There is no form of locking or control of when objects are added an deleted. In this iteration, it is important that
  users operate with extreme caution.
