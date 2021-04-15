#include "VanillaDataStructures.h"

#include "gtest/gtest.h"

TEST(VanillaDataStructuresTest, structSizes) { EXPECT_EQ(sizeof(VDS::SAVED_GAME_HEADER), 432); }
