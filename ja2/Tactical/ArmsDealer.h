// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef ARMS_DEALER_H
#define ARMS_DEALER_H

// enums for the various arms dealers
enum ArmsDealerID {
  ARMS_DEALER_INVALID = -1,
  ARMS_DEALER_BOBBYR = ARMS_DEALER_INVALID,

  ARMS_DEALER_FIRST = 0,
  ARMS_DEALER_TONY = ARMS_DEALER_FIRST,
  ARMS_DEALER_FRANZ,
  ARMS_DEALER_KEITH,
  ARMS_DEALER_JAKE,
  ARMS_DEALER_GABBY,

  ARMS_DEALER_DEVIN,
  ARMS_DEALER_HOWARD,
  ARMS_DEALER_SAM,
  ARMS_DEALER_FRANK,

  ARMS_DEALER_BAR_BRO_1,
  ARMS_DEALER_BAR_BRO_2,
  ARMS_DEALER_BAR_BRO_3,
  ARMS_DEALER_BAR_BRO_4,

  ARMS_DEALER_MICKY,

  ARMS_DEALER_ARNIE,
  ARMS_DEALER_FREDO,
  ARMS_DEALER_PERKO,

  // added only in GameVersion 54
  ARMS_DEALER_ELGIN,

  // added only in GameVersion 55
  ARMS_DEALER_MANNY,

  NUM_ARMS_DEALERS
};

static inline ArmsDealerID operator++(ArmsDealerID &a) { return a = (ArmsDealerID)(a + 1); }

#endif
