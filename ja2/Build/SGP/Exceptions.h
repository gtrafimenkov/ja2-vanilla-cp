// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#pragma once

#include <stdexcept>

class LibraryFileNotFoundException : public std::runtime_error {
 public:
  LibraryFileNotFoundException(const std::string &what_arg) : std::runtime_error(what_arg) {}
};
