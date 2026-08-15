# Console Chess ♟️

A two-player chess game that runs entirely in the terminal

Current version: **1.1.0**

## Requirements

- **C Compiler**: A C17-capable compiler such as `gcc` or `clang`.
- **Make**: The build is driven by a `Makefile`.
- **Nerd Font**: The terminal must support chess piece icons. Install a [Nerd Font](https://www.nerdfonts.com/) (e.g., JetBrains Mono Nerd Font) and set it as the terminal's font.
- **`VERSION`**: A file at the repository root declaring the version, required to build — see [Versioning](#versioning). Present when you clone the repository; include it manually if you package a source archive instead.

## Compile and Run

1. Open a terminal with a Nerd Font set as the default font.
2. Navigate to the project directory.
3. Build and play:
   ```
   make run
   ```

### Build targets

| Target       | What it does                                                          |
| ------------ | --------------------------------------------------------------------- |
| `make`       | Builds `./console-chess`                                              |
| `make run`   | Builds, then runs it                                                  |
| `make debug` | Builds `./console-chess-debug` with `-O0 -g3` and ASan/UBSan           |
| `make clean` | Removes `build/` and both binaries                                    |

Objects land in `build/`, one per source file, with header dependencies tracked
so editing a header rebuilds everything that includes it.

## Project layout

```
src/
├── main.c      startup, the menu, teardown
├── types.h     the shared types, and the only header other headers include
├── core/       board, move rules, captured-piece and move-history lists
├── ui/         everything that writes to the terminal
└── app/        turn flow and saving
```

Headers are included by their path below `src/`, e.g. `#include "core/board.h"`,
so each include says which layer it reaches into.

The split is `core/` versus everything else: `core/` holds the game itself and
touches no terminal and no files, which is what lets it be tested without one.


## How to Play

- **Keyboard input**: Enter moves using chess coordinates, letter first (e.g., `e2` to select/move to square e2).
- **Turn order**: White pieces play first, then Black. Turns alternate after each move.
- **Winning**: The game ends when a King is captured.

### Save and Load

- **Saving**: Players are prompted every 5 moves if they want to save the game. The game state is saved to `game_save.bin`.
- **Loading**: Select "Load Game" from the main menu when starting the program. The game will resume from where you left off.
- **Older save files**: The file records the game's structures directly, so it is tied to their layout. Each file carries a format version, and one written by an earlier build is refused with a message rather than misread. A rejected file is left on disk untouched.

## Versioning

Console Chess declares its version once, in `VERSION` at the repository root. Everything that reports a version — `console-chess --version`, the welcome banner, and saved games — derives from that one file; a build fails rather than compiling a binary with a missing or malformed version.

The project version and the save file's format version are separate numbers, tracking separate things:

- The **project version** (`VERSION`) identifies a feature release. It is what `--version` prints and what the welcome screen shows.
- The **save format version** (`SAVE_VERSION` in `src/app/save.c`) identifies the on-disk layout, and only changes when that layout changes. Whether a save file loads is decided by its format version, not by which release wrote it — see [Older save files](#save-and-load) above.

### Cutting a release

1. Edit `VERSION` with the new version number.
2. Commit the change.
3. Tag the commit `vX.Y.Z`.

## Notes

- Ensure your terminal supports Unicode and that the Nerd Font is correctly configured to display the chess piece icons.
- Check and checkmate are not enforced — the game ends when a King is actually captured.
