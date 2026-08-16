# Console Chess ♟️

A two-player chess game that runs entirely in the terminal

Current version: **2.0.1**

## Requirements

- **C Compiler**: A C17-capable compiler such as `gcc` or `clang`.
- **Make**: The build is driven by a `Makefile`.
- **Nerd Font**: The terminal must support chess piece icons. Install a [Nerd Font](https://www.nerdfonts.com/) (e.g., JetBrains Mono Nerd Font) and set it as the terminal's font — or run with `--ascii` and skip it.
- **A terminal**: the game draws a full-screen display and refuses to start if its input or output is not an interactive terminal.
- **`VERSION`**: A file at the repository root declaring the version, required to build — see [Versioning](#versioning). Present when you clone the repository; include it manually if you package a source archive instead.

## Compile and Run

1. Open a terminal with a Nerd Font set as the default font.
2. Navigate to the project directory.
3. Build and play:
   ```
   make run
   ```

### Options

| Option            | What it does                                                        |
| ----------------- | ------------------------------------------------------------------- |
| `--ascii`         | Draws pieces as letters (`KQRBNP` white, `kqrbnp` black) instead of icons, for terminals without a Nerd Font |
| `-v`, `--version` | Prints the version and exits                                        |
| `-h`, `--help`    | Prints the options and exits                                        |

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
├── main.c      startup, terminal setup, teardown
├── types.h     the shared types, and the only header other headers include
├── core/       board, move rules, captured-piece and move-history lists
├── ui/         the terminal layer: term, input, render, layout, glyphs
└── app/        the screen stack and the screens themselves
```

Headers are included by their path below `src/`, e.g. `#include "core/board.h"`,
so each include says which layer it reaches into.

The split is `core/` versus everything else: `core/` holds the game itself and
touches no terminal and no files, which is what lets it be tested without one.


## How to Play

- **Making a move**: three interchangeable ways to name a square, usable in any combination within the same move:
  - **Mouse**: click a piece, then click its destination.
  - **Typed coordinates**: type the square, press Enter, then type the destination and press Enter. Coordinates are letter first, e.g. `e2` then `e4`. What you type appears in the status bar; nothing scrolls.
  - **Keyboard cursor**: move the reversed-video square on the rank and file labels with the arrow keys, and press Enter to name it.

  Clicking a piece, typing a square, and moving the cursor all produce the same selection — whichever you used last is the one that counts, and none of them is required. Selecting is separate from moving: clicking or naming a square holding one of your own pieces only selects it, and a second click or square either completes the move, changes the selection to a different piece of yours, or — if it names an illegal square — is rejected and the selection stays. Escape, or naming the selected square again, cancels the selection.
- **Turn order**: White plays first, then Black. Between turns the board flips and the status bar waits for the next player to press Space, so a move stays on screen until someone is looking at it.
- **Highlights**: while a piece is selected, every square it may legally move to is marked — a centred dot on an empty square, a tinted background where it would capture (including en passant). The selected square keeps a dark olive tint, and the two squares of the move just played keep a dark green one. A king in check gets a dark amber tint plus a small `!` in the corner of its square, which stays in a final checkmate position. None of this changes what is legal — it is exactly what `generate_legal_moves` already decided, drawn. On a terminal without colour (or with `NO_COLOR` set), every one of these falls back to a distinct shape in the corner of the square instead of a tint, so nothing depends on colour to be readable. The tints are the `C_SQUARE_*` constants at the top of `src/app/game.c` if you want them lighter or darker.
- **Winning**: the game ends in checkmate, stalemate, or a draw by the fifty-move rule, insufficient material, or threefold repetition — whichever the position reaches first.

### Key bindings

| Key              | Action                                                          |
| ---------------- | ---------------------------------------------------------------- |
| Click             | Select a piece, or name its destination                          |
| Arrow keys        | Move the keyboard cursor                                         |
| `a`–`h`, `1`–`8`  | Type a square into the status bar's move field                   |
| `Enter`           | Submit the typed square, or the keyboard cursor if nothing is typed |
| `Backspace`       | Delete the last character typed, or release the selected piece   |
| `Esc`             | Clear the move field and the selection                           |
| `Space`           | Take the handoff and start your turn                             |
| `F`               | Flip the board's orientation                                     |
| `s`               | Save the game — only after a move has been played (temporary)    |
| `l`               | Load the saved game — only before the first move (temporary)     |
| `Ctrl-L`          | Force a full repaint                                             |
| `q`               | Quit                                                              |
| `Ctrl-C`          | Quit; the terminal is restored either way                        |

`F` is shifted because the lowercase letters `a`–`h` are file names and belong to
the move field.

### Notes on your terminal

- **Resizing**: the display is laid out for the terminal's current size every
  frame, including after a font size change. Below the size the board needs, the
  game is replaced by a message giving the required and current sizes; the game
  is untouched and returns when there is room again.
- **Text selection and the mouse**: with mouse reporting on, the terminal sends
  a click to the game instead of letting you select text with it. To select
  text from the game screen anyway, hold `Option` while dragging (macOS
  Terminal and iTerm2) or `Shift` (most Linux terminals). Mouse reporting is
  turned off the moment the game exits — normally, on a crash, or on Ctrl-C —
  so a terminal is never left in that state.
- **Scrolling**: the wheel does not scroll the game and does not type anything;
  it is read and deliberately discarded. The game runs on the alternate
  screen, so your shell's scrollback is untouched and returns intact when you
  quit.

### Save and Load

- **Saving**: `s` writes the game to `game_save.bin` and reports the result in
  the status bar — but only once a move has been played. Saving the opening
  position keeps nothing and would destroy a real save for it.
- **Loading**: `l` restores it — but **only on a new game, before the first move
  has been played**. A load replaces everything, and `l` sits one key away from
  the squares you spend the game typing, so it is withdrawn as soon as there is
  a game worth losing. The hint line shows `l load` exactly while it is
  available. Once a game is under way, the only way to load is to quit and start
  the program again. Saving and loading are therefore never offered at the same
  time: whichever applies to the game in front of you is the one you get.
- **When a load fails**, the status bar says why: no file, a file written by an
  incompatible build, or a file that is truncated or corrupt. A refused file is
  left on disk untouched.

Both bindings are **temporary**. The prompt-driven save and the main menu were
removed along with the scrolling display they were built on, and these keep the
capability reachable until autosave, a resume-on-launch prompt, and manual save
slots arrive with the app shell. The file format is being replaced at the same
time, so a save written now is unlikely to be readable by the build that ships
those — it will be refused, not misread.

## Versioning

Console Chess declares its version once, in `VERSION` at the repository root. Everything that reports a version — `console-chess --version`, the title bar, and saved games — derives from that one file; a build fails rather than compiling a binary with a missing or malformed version.

The project version and the save file's format version are separate numbers, tracking separate things:

- The **project version** (`VERSION`) identifies a feature release. It is what `--version` prints and what the title bar shows.
- The **save format version** (`SAVE_VERSION` in `src/app/save.c`) identifies the on-disk layout, and only changes when that layout changes. Whether a save file loads is decided by its format version, not by which release wrote it.

### Cutting a release

1. Edit `VERSION` with the new version number.
2. Commit the change.
3. Tag the commit `vX.Y.Z`.

## Notes

- Ensure your terminal supports Unicode and that the Nerd Font is correctly configured to display the chess piece icons. The game measures how wide your terminal draws them at startup and sizes the board to match, so the columns line up either way; `--ascii` avoids the question entirely.
- macOS and Linux. Windows would need a console-API backend.
