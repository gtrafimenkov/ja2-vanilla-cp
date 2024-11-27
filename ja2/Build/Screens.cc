#include "Screens.h"

#include "AniViewScreen.h"
#include "Credits.h"
#include "Editor/EditScreen.h"
#include "Editor/LoadScreen.h"
#include "FadeScreen.h"
#include "GameInitOptionsScreen.h"
#include "GameScreen.h"
#include "Intro.h"
#include "JAScreens.h"
#include "Laptop/Laptop.h"
#include "Macro.h"
#include "MainMenuScreen.h"
#include "MessageBoxScreen.h"
#include "OptionsScreen.h"
#include "SaveLoadScreen.h"
#include "Strategic/AIViewer.h"
#include "Strategic/AutoResolve.h"
#include "Strategic/MapScreen.h"
#include "Strategic/QuestDebugSystem.h"
#include "Tactical/ShopKeeperInterface.h"
#include "Utils/MapUtility.h"

Screens const GameScreens[] = {
    {EditScreenInit, EditScreenHandle, EditScreenShutdown},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, ErrorScreenHandle, NULL},  // Title Screen
    {NULL, InitScreenHandle, NULL},   // Title Screen
    {MainGameScreenInit, MainGameScreenHandle, MainGameScreenShutdown},
    {NULL, AniEditScreenHandle, NULL},
    {NULL, PalEditScreenHandle, NULL},
    {NULL, DebugScreenHandle, NULL},
    {MapScreenInit, MapScreenHandle, MapScreenShutdown},
    {LaptopScreenInit, LaptopScreenHandle, LaptopScreenShutdown},
    {NULL, LoadSaveScreenHandle, NULL},
    {NULL, MapUtilScreenHandle, NULL},
    {NULL, FadeScreenHandle, NULL},
    {NULL, MessageBoxScreenHandle, MessageBoxScreenShutdown},
    {NULL, MainMenuScreenHandle, NULL},
    {NULL, AutoResolveScreenHandle, NULL},
    {NULL, SaveLoadScreenHandle, NULL},
    {NULL, OptionsScreenHandle, NULL},
    {ShopKeeperScreenInit, ShopKeeperScreenHandle, ShopKeeperScreenShutdown},
    {NULL, SexScreenHandle, NULL},
    {NULL, GameInitOptionsScreenHandle, NULL},
    {NULL, NULL, NULL},
    {NULL, IntroScreenHandle, NULL},
    {NULL, CreditScreenHandle, NULL},
    {QuestDebugScreenInit, QuestDebugScreenHandle, NULL}};

#include "gtest/gtest.h"

TEST(Screens, asserts) { EXPECT_EQ(lengthof(GameScreens), MAX_SCREENS); }
