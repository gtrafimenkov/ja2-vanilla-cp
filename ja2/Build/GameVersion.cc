#include "GameVersion.h"

//
//	Keeps track of the game version
//

#ifndef BUILD_INFO
#define BUILD_INFO "unknown"
#endif

const char g_version_label[] = "JA2 Vanilla CP (" BUILD_INFO ")";

// This version is written into the save files.
// It should remain the same otherwise there will be warning on
// loading the game.
char const g_version_number[16] = "Build 04.12.02";

//
//		Keeps track of the saved game version.  Increment the saved game
// version whenever 	you will invalidate the saved game file

#define SAVE_GAME_VERSION 99

const uint32_t guiSavedGameVersion = SAVE_GAME_VERSION;

#include "gtest/gtest.h"

TEST(GameVersion, asserts) { EXPECT_EQ(lengthof(g_version_number), 16); }
