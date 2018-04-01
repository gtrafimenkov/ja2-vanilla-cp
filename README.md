# JA2 Vanilla Experience

The goal of the project is to provide vanilla experience to the fans of Jagged Alliance 2.

Out of the box the game should provide:
- the same look and feel as the original Jagged Alliance 2
- the same gameplay and balance

For the power users there is a way to customize the game with mods.

## How to start the game

1. Install original Jagged Alliance 2.

2. Check out this repository.

3. Build the game (see [compilation.md](compilation.md)) or download
   precompiled binaries from [releases](https://github.com/ja2-2019/ja2-ve/releases)
   and extract them into the repository.

4. Copy content of the installed Jagged Alliance 2 game into the repository.  You need to
   copy only Data folder - it is the folder containing slf files.

5. Start `ja2-ve.exe` on Windows, `ja2-ve` on Linux.

6. Enjoy the game.

If you have one of the localized versions of Jagged Alliance 2, for example, russian
version, you will need to pass `--resversion NAME` key to the executable.  See
`ja2-ve --help` for more info.

The repository also contain mods.  You can play a mod by passing parameter `--mod NAME`
to the executable.  List of available mods can be seen in folder `mods`.

## Supported localizations of Jagged Alliance 2

- Dutch
- English
- French
- German
- Italian
- Polish
- Russian (BUKA Agonia Vlasty)
- Russian (Gold)

## License

Original Jagged Alliance source codes were released by Strategy First Inc. in
2004 under Source Code License Agreement ("SFI-SCLA").  You can find the
license in file *SFI Source Code license agreement.txt*.

The license for changes before commit is not known.

All changes since `commit 8287b98` are released to the public domain.

All libraries in `_build/lib-*` have their own licenses.
