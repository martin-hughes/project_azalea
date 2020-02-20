"""Defines the mapfile class used to interpret an LD linker map file."""

from typing import IO
import re

class InvalidFileException(Exception):
  pass

class Symbol:
  def __init__(self, name, start_addr):
    self.name = name
    self.start_addr = start_addr
    self.end_addr = None

class mapfile:
  """Interprets an LD linker map file"""

  symbols = { }
  addresses = { }

  def __init__(self, read_obj : IO):
    raw_data = read_obj.read()

    lines = raw_data.split("\n")

    stage = 0
    last_symbol_name = None

    # Some regular expressions
    symbol_line = re.compile(r"^\s+(0x[\da-fA-F]+)\s+(.+)")

    for l in lines:
      l = l.strip("\r")

      if stage == 0:
        if l != "Archive member included to satisfy reference by file (symbol)":
          raise InvalidFileException

        stage = 1

      elif stage == 1:
        if l == "Allocating common symbols":
          stage = 2

        else:
          pass

      elif stage == 2:
        if l == "Discarded input sections":
          stage = 3

        else:
          pass

      elif stage == 3:
        if l == "Memory Configuration":
          stage = 4

        else:
          pass

      elif stage == 4:
        if l == "Linker script and memory map":
          stage = 5

        else:
          pass

      elif stage == 5:
        addr = None
        if l[0:2] == "  ":
          # Normal symbol line
          matches = symbol_line.match(l)

          try:
            addr = int(matches.groups()[0], base=16)
          except:
            print("FAIL: " +l)
            raise
          name = matches.groups()[1]

          self.symbols[name] = Symbol(name, addr)
          self.addresses[addr] = name

          if last_symbol_name is not None:
            self.symbols[last_symbol_name].end_addr = addr

          last_symbol_name = name

      else:
        raise InvalidFileException

    if stage != 5:
      raise InvalidFileException

  def find_symbol_by_addr(self, addr):
    found = None
    found_addr = 0
    for s in self.addresses:
      if s <= addr and s > found_addr:
        found = self.addresses[s]
        found_addr = s

    return self.symbols[found]
