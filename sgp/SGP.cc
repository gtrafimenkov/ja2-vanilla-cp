#include <exception>
#include <new>

#include <SDL.h>

#include "Build/GameLoop.h"
#include "Build/GameRes.h"
#include "Build/GameState.h"
#include "Build/Init.h"
#include "Build/Intro.h"
#include "Build/JA2_Splash.h"
#include "Build/SaveLoadGame.h"
#include "ModPackContentManager.h"
#include "sgp/Button_System.h"
#include "sgp/Debug.h"
#include "sgp/FileMan.h"
#include "sgp/Font.h"
#include "sgp/Input.h"
#include "sgp/Logger.h"
#include "sgp/MemMan.h"
#include "sgp/Random.h"
#include "sgp/SGP.h"
#include "sgp/SoundMan.h"
#include "sgp/Timer.h"
#include "sgp/UTF8String.h"
#include "sgp/Video.h"
#include "sgp/VObject.h"
#include "sgp/VSurface.h"
#include "slog/slog.h"
#include "src/DefaultContentManager.h"
#include "src/GameInstance.h"
#include "src/JsonUtility.h"
#include "src/policy/GamePolicy.h"

#define TAG "SGP"

#ifdef WITH_UNITTESTS
#include "gtest/gtest.h"
#endif

#if defined _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>

#	include "Local.h"
#endif

#include "Build/Utils/Multi_Language_Graphic_Utils.h"


extern BOOLEAN gfPauseDueToPlayerGamePause;


#define WITH_MODS (1)

////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////

// #include "src/JsonObject.h"
// #include "src/MagazineModel.h"
// #include "src/WeaponModels.h"
// #include "Build/Tactical/Weapons.h"
// #include "rapidjson/document.h"
// #include "rapidjson/filestream.h"
// #include "rapidjson/prettywriter.h"
// #include "stdio.h"

// bool writeWeaponsToJson(const char *name/*, const struct WEAPONTYPE *weapon*/, int weaponCount)
// {
//   FILE *f = fopen(name, "wt");
//   if(f)
//   {
//     rapidjson::FileStream os(f);
//     rapidjson::PrettyWriter<rapidjson::FileStream> writer(os);

//     rapidjson::Document document;
//     document.SetArray();
//     rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

//     for(int i = 0; i < weaponCount; i++)
//     {
//       // printf("%d\n", i);
//       const WeaponModel *w = GCM->getWeapon(i);
//       JsonObject obj(allocator);
//       w->serializeTo(obj);
//       document.PushBack(obj.getValue(), allocator);
//     }

//     document.Accept(writer);

//     fputs("\n", f);
//     return fclose(f) == 0;
//   }
//   return false;
// }

// bool writeMagazinesToJson(const char *name)
// {
//   FILE *f = fopen(name, "wt");
//   if(f)
//   {
//     rapidjson::FileStream os(f);
//     rapidjson::PrettyWriter<rapidjson::FileStream> writer(os);

//     rapidjson::Document document;
//     document.SetArray();
//     rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

//     const std::vector<const MagazineModel*>& magazines = GCM->getMagazines();
//     for(const MagazineModel* mag: magazines)
//     {
//       JsonObject obj(allocator);
//       mag->serializeTo(obj);
//       document.PushBack(obj.getValue(), allocator);
//     }

//     document.Accept(writer);

//     fputs("\n", f);
//     return fclose(f) == 0;
//   }
//   return false;
// }

////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////

static BOOLEAN gfGameInitialized = FALSE;


static bool getResourceVersion(const char *versionName, GameVersion *version);
static std::string findRootGameResFolder(const std::string &configPath);
static void WriteDefaultConfigFile(const char* ConfigFile);

static void convertDialogQuotesToJson(const DefaultContentManager *cm,
                                      STRING_ENC_TYPE encType,
                                      const char *dialogFile, const char *outputFile);

/** Deinitialize the game an exit. */
static void deinitGameAndExit()
{
	FastDebugMsg("Exiting Game");

	SoundServiceStreams();

	if (gfGameInitialized)
	{
		ShutdownGame();
	}

	ShutdownButtonSystem();
	MSYS_Shutdown();

#ifndef UTIL
  ShutdownSoundManager();
#endif

	ShutdownVideoSurfaceManager();
  ShutdownVideoObjectManager();
  ShutdownVideoManager();

  ShutdownMemoryManager();  // must go last, for MemDebugCounter to work right...

  SDL_Quit();

  exit(0);
}


/** Request game exit.
 * Call this function if you want to exit the game. */
void requestGameExit()
{
  SDL_Event event;
  event.type = SDL_QUIT;
  SDL_PushEvent(&event);
}

static void MainLoop(int msPerGameCycle)
{
	BOOLEAN s_doGameCycles = TRUE;

  while (true)
  {
    // cycle until SDL_Quit is received

		SDL_Event event;
		if (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_APP_WILLENTERBACKGROUND:
					s_doGameCycles = false;
					break;

				case SDL_APP_WILLENTERFOREGROUND:
					s_doGameCycles = true;
					break;

				case SDL_KEYDOWN: KeyDown(&event.key.keysym); break;
				case SDL_KEYUP:   KeyUp(  &event.key.keysym); break;
				case SDL_TEXTINPUT: TextInput(&event.text); break;

				case SDL_MOUSEBUTTONDOWN: MouseButtonDown(&event.button); break;
				case SDL_MOUSEBUTTONUP:   MouseButtonUp(&event.button);   break;

				case SDL_MOUSEMOTION:
					SetSafeMousePosition(event.motion.x, event.motion.y);
					break;

				case SDL_MOUSEWHEEL: MouseWheelScroll(&event.wheel); break;

				case SDL_QUIT: deinitGameAndExit(); break;
			}
		}
		else
		{
			if (s_doGameCycles)
			{
        UINT32 gameCycleMS = GetClock();
#if DEBUG_PRINT_GAME_CYCLE_TIME
        UINT32 totalGameCycleMS = gameCycleMS;
#endif
				GameLoop();
        gameCycleMS = GetClock() - gameCycleMS;

        if(gameCycleMS < msPerGameCycle)
        {
          SDL_Delay(msPerGameCycle - gameCycleMS);
        }

#if DEBUG_PRINT_GAME_CYCLE_TIME
        totalGameCycleMS = GetClock() - totalGameCycleMS;
        printf("game cycle: %4d %4d\n", gameCycleMS, totalGameCycleMS);
#endif
			}
			else
			{
				SDL_WaitEvent(NULL);
			}
		}
  }
}


static int Failure(char const* const msg, bool showInfoIcon=false)
{
	fprintf(stderr, "%s\n", msg);
#if defined _WIN32
	MessageBox(0, msg, APPLICATION_NAME, MB_OK | (showInfoIcon ? MB_ICONINFORMATION : MB_ICONERROR) | MB_TASKMODAL);
#endif
	return EXIT_FAILURE;
}

////////////////////////////////////////////////////////////

ContentManager *GCM = NULL;

////////////////////////////////////////////////////////////

struct CommandLineParams
{
  CommandLineParams()
  {
#ifdef WITH_MODS
    useMod = false;
#endif
    doUnitTests = false;
    showDebugMessages = false;
    resourceVersionGiven = false;
    gameDirPathGiven = false;
  }

#ifdef WITH_MODS
  bool useMod;
  std::string modName;
#endif

  bool resourceVersionGiven;
  std::string resourceVersion;

  bool doUnitTests;
  bool showDebugMessages;

  bool gameDirPathGiven;
  std::string gameDirPath;
};

static BOOLEAN ParseParameters(int argc, char* const argv[],
                               CommandLineParams *params);

int main(int argc, char* argv[])
try
{
  std::string exeFolder = FileMan::getParentPath(argv[0], true);

  // init logging
  SLOG_Init(SLOG_STDERR, "ja2.log");
  SLOG_SetLevel(SLOG_WARNING, SLOG_WARNING);

  setGameVersion(GV_ENGLISH);

  CommandLineParams params;
	if (!ParseParameters(argc, argv, &params)) return EXIT_FAILURE;

  if(params.showDebugMessages)
  {
    SLOG_SetLevel(SLOG_DEBUG, SLOG_DEBUG);
  }

#ifdef WITH_UNITTESTS
  if(params.doUnitTests)
  {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
  }
#endif

  GameVersion version = GV_ENGLISH;
  if(params.resourceVersionGiven)
  {
    if(!getResourceVersion(params.resourceVersion.c_str(), &version))
    {
      SLOGE(TAG, "Unknown version of the game: %s\n", params.resourceVersion.c_str());
      return EXIT_FAILURE;
    }
  }
  setGameVersion(version);

  ////////////////////////////////////////////////////////////

	SDL_Init(SDL_INIT_VIDEO);

  // restore output to the console (on windows when built with MINGW)
#ifdef __MINGW32__
  freopen("CON", "w", stdout);
  freopen("CON", "w", stderr);
#endif

#ifdef SGP_DEBUG
	// Initialize the Debug Manager - success doesn't matter
	InitializeDebugManager();
#endif

  // this one needs to go ahead of all others (except Debug), for MemDebugCounter to work right...
	FastDebugMsg("Initializing Memory Manager");
	InitializeMemoryManager();

  FastDebugMsg("Initializing Game Resources");
  if(!params.gameDirPathGiven) {
    params.gameDirPath = exeFolder;
  }

  std::string extraDataDir = exeFolder;

  std::string externalizedDataPath = FileMan::joinPaths(extraDataDir, "externalized");

  DefaultContentManager *cm;

#ifdef WITH_MODS
  if(params.useMod)
  {
    std::string modName = params.modName;
    std::string modResFolder = FileMan::joinPaths(FileMan::joinPaths(FileMan::joinPaths(extraDataDir, "mods"), modName), "data");
    cm = new ModPackContentManager(version,
                                   modName, modResFolder, configFolderPath,
                                   configPath, gameResRootPath,
                                   externalizedDataPath);
    LOG_INFO("------------------------------------------------------------------------------\n");
    LOG_INFO("Root game resources directory: '%s'\n", params.gameDirPath.c_str());
    LOG_INFO("Extra data directory:          '%s'\n", extraDataDir.c_str());
    LOG_INFO("Data directory:                '%s'\n", cm->getDataDir().c_str());
    LOG_INFO("Tilecache directory:           '%s'\n", cm->getTileDir().c_str());
    LOG_INFO("Saved games directory:         '%s'\n", cm->getSavedGamesFolder().c_str());
    LOG_INFO("------------------------------------------------------------------------------\n");
    LOG_INFO("MOD name:                      '%s'\n", modName.c_str());
    LOG_INFO("MOD resource directory:        '%s'\n", modResFolder.c_str());
    LOG_INFO("------------------------------------------------------------------------------\n");
  }
  else
#endif
  {
    cm = new DefaultContentManager(version,
                                   configFolderPath, configPath,
                                   gameResRootPath, externalizedDataPath);
    LOG_INFO("------------------------------------------------------------------------------\n");
    LOG_INFO("Root game resources directory: '%s'\n", params.gameDirPath.c_str());
    LOG_INFO("Extra data directory:          '%s'\n", extraDataDir.c_str());
    LOG_INFO("Data directory:                '%s'\n", cm->getDataDir().c_str());
    LOG_INFO("Tilecache directory:           '%s'\n", cm->getTileDir().c_str());
    LOG_INFO("Saved games directory:         '%s'\n", cm->getSavedGamesFolder().c_str());
    LOG_INFO("------------------------------------------------------------------------------\n");
  }

  if(!cm->loadGameData())
  {
    LOG_INFO("Failed to load the game data.\n");
  }
  else
  {

    GCM = cm;

    FastDebugMsg("Initializing Video Manager");
    InitializeVideoManager();

    FastDebugMsg("Initializing Video Object Manager");
    InitializeVideoObjectManager();

    FastDebugMsg("Initializing Video Surface Manager");
    InitializeVideoSurfaceManager();

    InitJA2SplashScreen();

    // Initialize Font Manager
    FastDebugMsg("Initializing the Font Manager");
    // Init the manager and copy the TransTable stuff into it.
    InitializeFontManager();

    FastDebugMsg("Initializing Sound Manager");
#ifndef UTIL
    InitializeSoundManager();
#endif

    FastDebugMsg("Initializing Random");
    // Initialize random number generator
    InitializeRandom(); // no Shutdown

    FastDebugMsg("Initializing Game Manager");
    // Initialize the Game
    InitializeGame();

    gfGameInitialized = TRUE;

    ////////////////////////////////////////////////////////////

    // some data convertion
    // convertDialogQuotesToJson(cm, SE_RUSSIAN, "mercedt/051.edt", FileMan::joinPaths(exeFolder, "051.edt.json").c_str());
    // convertDialogQuotesToJson(cm, SE_RUSSIAN, "mercedt/052.edt", FileMan::joinPaths(exeFolder, "052.edt.json").c_str());
    // convertDialogQuotesToJson(cm, SE_RUSSIAN, "mercedt/055.edt", FileMan::joinPaths(exeFolder, "055.edt.json").c_str());

    // writeWeaponsToJson(FileMan::joinPaths(exeFolder, "externalized/weapons.json").c_str(), MAX_WEAPONS+1);
    // writeMagazinesToJson(FileMan::joinPaths(exeFolder, "externalized/magazines.json").c_str());

    // readWeaponsFromJson(FileMan::joinPaths(exeFolder, "weapon.json").c_str());
    // readWeaponsFromJson(FileMan::joinPaths(exeFolder, "weapon2.json").c_str());

    ////////////////////////////////////////////////////////////

    if(isEnglishVersion())
    {
      SetIntroType(INTRO_SPLASH);
    }

    FastDebugMsg("Running Game");

    /* At this point the SGP is set up, which means all I/O, Memory, tools, etc.
     * are available. All we need to do is attend to the gaming mechanics
     * themselves */
    MainLoop(GCM->getGamePolicy()->ms_per_game_cycle);
  }

  SLOG_Deinit();

  delete cm;
  GCM = NULL;

	return EXIT_SUCCESS;
}
catch (const std::bad_alloc&)
{
	return Failure("ERROR: out of memory");
}
catch (const LibraryFileNotFoundException& e)
{
	return Failure(e.what(), true);
}
catch (const std::exception& e)
{
	char msg[2048];
	snprintf(msg, lengthof(msg), "ERROR: caught unhandled exception:\n%s", e.what());
	return Failure(msg);
}
catch (...)
{
	return Failure("ERROR: caught unhandled unknown exception");
}


/** Set game resources version. */
static bool getResourceVersion(const char *versionName, GameVersion *version)
{
  if(strcasecmp(versionName, "ENGLISH") == 0)
  {
    *version = GV_ENGLISH;
  }
  else if(strcasecmp(versionName, "DUTCH") == 0)
  {
    *version = GV_DUTCH;
  }
  else if(strcasecmp(versionName, "FRENCH") == 0)
  {
    *version = GV_FRENCH;
  }
  else if(strcasecmp(versionName, "GERMAN") == 0)
  {
    *version = GV_GERMAN;
  }
  else if(strcasecmp(versionName, "ITALIAN") == 0)
  {
    *version = GV_ITALIAN;
  }
  else if(strcasecmp(versionName, "POLISH") == 0)
  {
    *version = GV_POLISH;
  }
  else if(strcasecmp(versionName, "RUSSIAN") == 0)
  {
    *version = GV_RUSSIAN;
  }
  else if(strcasecmp(versionName, "RUSSIAN_GOLD") == 0)
  {
    *version = GV_RUSSIAN_GOLD;
  }
  else
  {
    return false;
  }
  return true;
}

static BOOLEAN ParseParameters(int argc, char* const argv[],
                               CommandLineParams *params)
{
	const char* const name = *argv;
	if (name == NULL) return TRUE; // argv does not even contain the program name

	BOOLEAN success = TRUE;
  for(int i = 1; i < argc; i++)
  {
    bool haveNextParameter = (i + 1) < argc;

		if (strcmp(argv[i], "--nosound") == 0)
		{
			SoundEnableSound(FALSE);
		}
#ifdef WITH_UNITTESTS
    else if (strcmp(argv[i], "--unittests") == 0)
    {
      params->doUnitTests = true;
      return true;
    }
#endif
    else if (strcmp(argv[i], "--debug") == 0)
    {
      params->showDebugMessages = true;
      return true;
    }
#ifdef WITH_MODS
    else if (strcmp(argv[i], "--mod") == 0)
    {
      if(haveNextParameter)
      {
        params->useMod = true;
        params->modName = argv[++i];
      }
      else
      {
        LOG_ERROR("Missing value for command-line key '--mod'\n");
        success = FALSE;
      }
    }
#endif
    else if (strcmp(argv[i], "--gamedir") == 0)
    {
      if(haveNextParameter)
      {
        params->gameDirPathGiven = true;
        params->gameDirPath = argv[++i];
      }
      else
      {
        LOG_ERROR("Missing value for command-line key '-gamedir'\n");
        success = FALSE;
      }
    }
		else if (strcmp(argv[i], "--editor") == 0)
		{
      GameState::getInstance()->setEditorMode(false);
		}
		else if (strcmp(argv[i], "--editorauto") == 0)
		{
      GameState::getInstance()->setEditorMode(true);
		}
    else if (strcmp(argv[i], "--resversion") == 0)
    {
      if(haveNextParameter)
      {
        params->resourceVersionGiven = true;
        params->resourceVersion = argv[++i];
      }
      else
      {
        LOG_ERROR("Missing value for command-line key '-resversion'\n");
        success = FALSE;
      }
    }
		else
		{
			if (strcmp(argv[i], "--help") != 0)
			{
				fprintf(stderr, "Unknown switch \"%s\"\n", argv[i]);
			}
			success = FALSE;
		}
	}

	if (!success)
	{
		fprintf(stderr,
			"Usage: %s [options]\n"
			"\n"
			"  --resversion  Version of the game resources.\n"
			"                Possible values: DUTCH, ENGLISH, FRENCH, GERMAN, ITALIAN, POLISH, RUSSIAN, RUSSIAN_GOLD\n"
			"                Default value is ENGLISH\n"
			"                RUSSIAN is for BUKA Agonia Vlasty release\n"
			"                RUSSIAN_GOLD is for Gold release\n"
#ifdef WITH_MODS
      "\n"
      "  --mod NAME    Start one of the game modifications, bundled into the game.\n"
      "                NAME is the name of modification, e.g. 'from-russia-with-love'.\n"
      "                See folder mods for possible options\n"
#endif
			"\n"
      "  --gamedir     Directory where the original game resources can be found.\n"
      "                By default the directory where the executable file is located.\n"
			"\n"
			"  --debug       Show debug messages\n"
#ifdef WITH_UNITTESTS
			"\n"
      "  --unittests   Perform unit tests\n"
      "                  ja2-ve --unittests [gtest options]\n"
      "                  E.g. ja2-ve --unittests --gtest_output=\"xml:report.xml\" --gtest_repeat=2\n"
			"\n"
#endif
			"  --editor      Start the map editor (Editor.slf is required)\n"
			"  --editorauto  Start the map editor and load sector A9 (Editor.slf is required)\n"
			"  --help        Display this information\n"
			"  --nosound     Turn the sound and music off\n"
            ,
			name
		);
	}
	return success;
}

static void convertDialogQuotesToJson(const DefaultContentManager *cm,
                                      STRING_ENC_TYPE encType,
                                      const char *dialogFile, const char *outputFile)
{
  std::vector<UTF8String*> quotes;
  std::vector<std::string> quotes_str;
  cm->loadAllDialogQuotes(encType, dialogFile, quotes);
  for(int i = 0; i < quotes.size(); i++)
  {
    quotes_str.push_back(std::string(quotes[i]->getUTF8()));
    delete quotes[i];
    quotes[i] = NULL;
  }
  JsonUtility::writeToFile(outputFile, quotes_str);
}

////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////
