// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#pragma once

#include <stdint.h>

/** Encoding corrector interface.
 *
 * Encoding corrector is used to change (correct) incorrectly
 * encoded into UTF-16 strings. */
class IEncodingCorrector {
 public:
  /** Fix one UTF-16 code point. */
  virtual uint16_t fix(uint16_t codePoint) const = 0;
};
