#include <exception>
#include <new>

#include "slog/slog.h"

#include "sgp/Button_System.h"
#include "sgp/Debug.h"
#include "sgp/FileMan.h"
#include "sgp/Font.h"
#include "GameLoop.h"
#include "Init.h" // XXX should not be used in SGP
#include "sgp/Input.h"
#include "Intro.h"
#include "JA2_Splash.h"
#include "sgp/MemMan.h"
#include "sgp/Random.h"
#include "sgp/SGP.h"
#include "SaveLoadGame.h" // XXX should not be used in SGP
#include "sgp/SoundMan.h"
#include "sgp/VObject.h"
#include "sgp/Video.h"
#include "sgp/VSurface.h"
#include <SDL.h>
#include "GameRes.h"
#include "sgp/Logger.h"
#include "GameState.h"
#include "sgp/Exceptions.h"
#include "sgp/Timer.h"

#ifdef WITH_UNITTESTS
#include "gtest/gtest.h"
#endif

#if defined _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>

#	include "Local.h"
#endif

#include "Utils/Multi_Language_Graphic_Utils.h"


extern BOOLEAN gfPauseDueToPlayerGamePause;


/**
 * Number of milliseconds for one game cycle.
 * 25 ms gives approx. 40 cycles per second (and 40 frames per second, since the screen
 * is updated on every cycle). */
#define MS_PER_GAME_CYCLE               (25)


static BOOLEAN gfGameInitialized = FALSE;


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

  ShutdownSoundManager();

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

static void MainLoop()
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
				GameLoop();
        gameCycleMS = GetClock() - gameCycleMS;

        if(gameCycleMS < MS_PER_GAME_CYCLE)
        {
          SDL_Delay(MS_PER_GAME_CYCLE - gameCycleMS);
        }
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

struct CommandLineParams
{
  CommandLineParams()
    : success(false), resourceVersionGiven(false), doUnitTests(false)
  {
  }

  bool success;
  bool resourceVersionGiven;
  bool doUnitTests;
  std::string resourceVersion;
};

static CommandLineParams ParseParameters(int argc, char* const argv[]);


int main(int argc, char* argv[])
try
{
  std::string exeFolder = FileMan::getParentPath(argv[0], true);

  setGameVersion(GV_ENGLISH);

  CommandLineParams params = ParseParameters(argc, argv);

	if (!params.success) return EXIT_FAILURE;

#ifdef WITH_UNITTESTS
  if(params.doUnitTests)
  {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
  }
#endif

	SDL_Init(SDL_INIT_VIDEO);

  // restore output to the console (on windows when built with MINGW)
#ifdef __MINGW32__
  freopen("CON", "w", stdout);
  freopen("CON", "w", stderr);
#endif

	InitializeMemoryManager();
	InitializeFileManager();
  InitializeVideoManager();
  InitializeVideoObjectManager();
  InitializeVideoSurfaceManager();
  InitGameResources();
  InitJA2SplashScreen();
  InitializeFontManager();
  InitializeSoundManager();
  InitializeRandom();
  InitializeGame();

  gfGameInitialized = TRUE;

  if(isEnglishVersion())
  {
    SetIntroType(INTRO_SPLASH);
  }

  MainLoop();
  SLOG_Deinit();
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
static BOOLEAN setResourceVersion(const char *version)
{
  if(strcasecmp(version, "ENGLISH") == 0)
  {
    setGameVersion(GV_ENGLISH);
  }
  else if(strcasecmp(version, "DUTCH") == 0)
  {
    setGameVersion(GV_DUTCH);
  }
  else if(strcasecmp(version, "FRENCH") == 0)
  {
    setGameVersion(GV_FRENCH);
  }
  else if(strcasecmp(version, "GERMAN") == 0)
  {
    setGameVersion(GV_GERMAN);
  }
  else if(strcasecmp(version, "ITALIAN") == 0)
  {
    setGameVersion(GV_ITALIAN);
  }
  else if(strcasecmp(version, "POLISH") == 0)
  {
    setGameVersion(GV_POLISH);
  }
  else if(strcasecmp(version, "RUSSIAN") == 0)
  {
    setGameVersion(GV_RUSSIAN);
  }
  else if(strcasecmp(version, "RUSSIAN_GOLD") == 0)
  {
    setGameVersion(GV_RUSSIAN_GOLD);
  }
  else
  {
    LOG_ERROR("Unknown version of the game: %s\n", version);
    return false;
  }
  LOG_INFO("Game version: %s\n", version);
  return true;
}

static CommandLineParams ParseParameters(int argc, char* const argv[])
{
  CommandLineParams params;
  params.success = true;
  for(int i = 1; i < argc; i++)
  {
    bool haveNextParameter = (i + 1) < argc;

		if (strcmp(argv[i], "--editor") == 0)
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
        params.success = setResourceVersion(argv[++i]);
      }
      else
      {
        LOG_ERROR("Missing value for command-line key '-resversion'\n");
        params.success = FALSE;
      }
    }
#ifdef WITH_UNITTESTS
    else if (strcmp(argv[i], "--unittests") == 0)
    {
      params.doUnitTests = true;
      break;
    }
#endif
		else
		{
			if (strcmp(argv[i], "--help") != 0)
			{
				fprintf(stderr, "Unknown switch \"%s\"\n", argv[i]);
			}
			params.success = FALSE;
      break;
		}
	}

	if (!params.success)
	{
		fprintf(stderr,
			"Usage: %s [options]\n"
			"\n"
			"  --resversion  Version of the game resources.\n"
			"                Possible values: DUTCH, ENGLISH, FRENCH, GERMAN, ITALIAN, POLISH, RUSSIAN, RUSSIAN_GOLD\n"
			"                Default value is ENGLISH\n"
			"                RUSSIAN is for BUKA Agonia Vlasty release\n"
			"                RUSSIAN_GOLD is for Gold release\n"
			"  --editor      Start the map editor (Editor.slf is required)\n"
			"  --editorauto  Start the map editor and load sector A9 (Editor.slf is required)\n"
#ifdef WITH_UNITTESTS
      "  --unittests   Perform unit tests\n"
#endif
			"  --help        Display this information\n",
			argv[0]
		);
	}
	return params;
}
