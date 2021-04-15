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

## How to build

You will need a Linux based system:

```
make install-build-dependencies-linux
make -j$(nproc)
```

## How to play the game

- install the original version of the game (from the original game CDs, Steam, gog.com, etc.)
- copy the builded binary to the game directory alongside the original ja2.exe
- launch the builded binary

## License

- the original Jagged Alliance 2 source codes were released by Strategy First Inc. in
  2004 under Source Code License Agreement ("SFI-SCLA").  See [SFI-SCLA](SFI-SCLA.txt)
- changes since `commit 8287b98` are released to the public domain, but only changes
  themselves.  The whole work is still subject to SFI-SCLA
- all libraries in `libs` have their own licenses
