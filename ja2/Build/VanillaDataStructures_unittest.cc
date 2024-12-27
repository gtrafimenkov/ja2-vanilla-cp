// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "VanillaDataStructures.h"

#include "gtest/gtest.h"

TEST(VanillaDataStructuresTest, structSizes) { EXPECT_EQ(sizeof(VDS::SAVED_GAME_HEADER), 432); }
