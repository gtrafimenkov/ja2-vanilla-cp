# Jagged Alliance 2 Vanilla Cross-Platform

This project is for maintaining Jagged Alliance 2 source codes
in buildable and workable state.  This project has
the same goal as [gtrafimenkov/ja2-vanilla](https://github.com/gtrafimenkov/ja2-vanilla),
but is based on [ja2-stracciatella](https://github.com/ja2-stracciatella/ja2-stracciatella)
codebase.

Goals:
- keep the sources buildable with modern development tools
- keep the game playable reasonably well on modern versions of Windows and Linux

These sources are very easy to change, but it's even easier to introduce
bugs by doing it.

Hence, the rules:
- keep changes to the minimum
- fix only critial bugs and issues
- don't change sources for the sake of it
- don't introduce any new features

This project is only for maintaining the vanilla game.

## Project structure

```
ja2                - game sources
libs               - third-party libraries
```

## Build requirements

- CMake
- GCC or Clang for Linux
- Visual Studio Community 2022 for Windows

## How to build, test, and run

```
python xx.py build-debug test-debug copy-data run
```

## How to play the game

- install the original version of the game (from the original game CDs, Steam, gog.com, etc.)
- `python xx.py build-debug test-debug`
- `python xx.py copy-data run` or copy `build/Debug/ja2vcp.exe` to the game directory
   alongside the original ja2.exe and run `ja2vcp.exe`

The game is tested on Windows 10.

## License

This is not open source as defined by [The Open Source Definition](https://opensource.org/osd/).
The original JA2 sources were released under the terms of [SFI-SCLA](SFI-SCLA.txt) license.
Please read it carefully and make your own mind regarding this.
