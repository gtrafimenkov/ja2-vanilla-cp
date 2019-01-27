# JA2 Vanilla Experience

Original experience for the fans of Jagged Alliance 2:
  - the same look and feel as the original game
  - the same gameplay and balance

You will need game data of the original game.

Supported platforms:
  - Windows
  - Linux (Ubuntu 18.04)

## How to start the game

1. Clone the repository.

2. Install original Jagged Alliance 2 and copy `Data` folder from it.

3. Compile the game according to instructions [compilation.md](compilation.md).

4. Start the game with command `./ja2-ve` (on Linux) or `ja2-ve.exe` (on Windows).

## Support of localized version of the game

If you have one of the localized versions of Jagged Alliance 2, for example, russian
version, you will need to pass `--resversion NAME` key to the executable.  See
`ja2-ve --help` for more info.

## Supported localizations of Jagged Alliance 2

- Dutch
- English
- French
- German
- Italian
- Polish
- Russian (BUKA Agonia Vlasty)
- Russian (Gold)

## Bug reports and pull requests

Bug reports are welcome: please file an issue.  Bugs of the original game will probably not
be fixed though because the goal of the project is to provide the vanilla experience and
it's very easy to introduce new bugs.

Pull requests with bug fixes are welcome.  Pull requests with new features and modifications
are not welcome because the goal of the project is to provide the vanilla experience.

## Other projects

- [ja2-stracciatella](https://github.com/ja2-stracciatella/ja2-stracciatella)

## License

Original Jagged Alliance source codes were released by Strategy First Inc. in
2004 under Source Code License Agreement ("SFI-SCLA").  You can find the
license in file *SFI Source Code license agreement.txt*.

The license for changes before `commit 8287b98` is not known.

All changes since `commit 8287b98` are released to the public domain.

All libraries in `_build/lib-*` have their own licenses.
