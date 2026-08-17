# Console Chess ♟️

A two-player chess game that runs entirely in the terminal

Current version: **3.0.0**

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

The program opens on a main menu: New Game, Load Game, How to Play, Settings,
Quit. Every screen that offers a choice works the same way, with no
exceptions — this one, the result screen, Settings, the saved-game list, and
the in-game overlays alike (confirming a resignation, the answer to a draw
offer, what a pawn promotes to, what to do about an unsaved game when you
quit).
Arrow keys move the highlight, Enter chooses it, and clicking an option only
highlights it too, so a stray click can never be mistaken for a confirmed
choice — which matters most for exactly the options that end a game. Those
screens go one further and open with the highlight already on the harmless
option: "No" on a resignation or a draw agreement, "Cancel" on the quit
choice, so a reflexive Enter costs you nothing and going through with it is
one arrow key. None of them have letter shortcuts: one gesture works
everywhere instead of a small vocabulary per overlay. In-game commands (below) are a different thing and
are single-key — they open a screen, they do not decide anything.

Whatever currently has the keyboard says how to drive it on the bottom row of
the screen, and only there. An overlay's hint replaces the game's command
list for as long as the overlay is up.

- **Making a move**: three interchangeable ways to name a square, usable in any combination within the same move:
  - **Mouse**: click a piece, then click its destination.
  - **Typed coordinates**: type the square, press Enter, then type the destination and press Enter. Coordinates are letter first, e.g. `e2` then `e4`. What you type appears in the status bar; nothing scrolls.
  - **Board cursor**: press any arrow key to bring it up — it starts at the centre of the board, so nothing is more than four steps away — then move it with the arrows and press Enter to name the square it sits on.

  Clicking a piece, typing a square, and moving the cursor all produce the same selection — whichever you used last is the one that counts, and none of them is required. Selecting is separate from moving: clicking or naming a square holding one of your own pieces only selects it, and a second click or square either completes the move, changes the selection to a different piece of yours, or — if it names an illegal square — is rejected and the selection stays. Escape, or naming the selected square again, cancels the selection.

  The cursor is only there while you are using it. Each turn starts with nothing selected and nothing highlighted; the cursor marks the square it is on and that square's rank and file labels, and goes away once the move is played or the selection is cancelled. All three ways of naming a square move it, so a highlighted rank and file always tell you which square is in hand — including the one you just clicked.
- **Turn order**: White plays first, then Black. Between turns the board flips and the status bar waits for the next player to press Space before it does, so a move stays on screen until someone is looking at it — this is the only way the board ever changes orientation — there is no manual flip, since seeing the position from the other side is not something either player is entitled to mid-game — and it always happens; there is no setting to skip it, since the same gesture is meant to double as the "I'm ready" signal a future timed mode will need.
- **Resigning and draws**: `x` resigns the side whose turn it is, after a confirmation naming who that hands the win to; there is no separate "which side" question, since the handover has already established whose turn it is. `o` offers a draw and opens the other side's answer in the same step — the question names who offered and who is answering, since in pass-and-play both of you are using one keyboard. Playing a move instead of answering counts as a decline.
- **Move history**: `H` (shift) opens the full move list in standard algebraic notation, numbered and paired by turn, scrollable with the arrow keys, page keys, Home/End, or the mouse wheel. It is reachable during play and from the result screen, and never changes the position. There is no undo or redo during play — chess does not allow taking back a move you have already made — but the move list is kept in enough detail that a future "replay a finished game" mode can step back and forward through it.
- **Highlights**: while a piece is selected, every square it may legally move to is marked — a small centred dot on an empty square, a tinted background where it would capture (including en passant). The selected square keeps a tint, and the two squares of the move just played keep another. A king in check gets a tinted square plus a small `!` in the corner — which stays visible on the result screen too, showing exactly which king was mated, not just that the game ended. None of this changes what is legal — it is exactly what `generate_legal_moves` already decided, drawn. On a terminal without colour (or with `NO_COLOR` set), every one of these falls back to a distinct shape in the corner of the square instead of a tint, so nothing depends on colour to be readable. The tints themselves are one of the colour schemes in Settings.
- **Winning**: the game ends in checkmate, stalemate, a draw by the fifty-move rule, insufficient material, or threefold repetition, or by resignation or an agreed draw — whichever comes first. The result screen shows the outcome, the reason, and the final position exactly as it was left (same orientation, king in check still marked if the game ended in mate), and offers a new game, reviewing the history, or returning to the menu.

### Key bindings

In-game commands are always shown in the status bar, dimmed when they currently do nothing rather than hidden.

| Key              | Action                                                          |
| ---------------- | ---------------------------------------------------------------- |
| Click             | Select a piece, or name its destination                          |
| Arrow keys        | Bring up the board cursor, then move it                          |
| `a`–`h`, `1`–`8`  | Type a square into the status bar's move field                   |
| `Enter`           | Submit the typed square, or the board cursor if nothing is typed |
| `Backspace`       | Delete the last character typed, or release the selected piece   |
| `Esc`             | Clear the move field and the selection                           |
| `Space`           | Take the handoff and start your turn                             |
| `s`               | Save the game (only once a move has been played)                 |
| `H`               | Open the move history                                            |
| `x`               | Resign — the side to move, after confirming                      |
| `o`               | Offer a draw (the other side answers straight away)              |
| `?`               | Help                                                              |
| `Ctrl-L`          | Force a full repaint                                             |
| `q`               | Quit — asks to save and quit, quit without saving, or cancel     |
| `Ctrl-C`          | Quit immediately, without saving; the terminal is restored either way |

`H` is shifted, and `s`, `x`, `o` are all outside `a`–`h`, because those
lowercase letters are file names and belong to the move field.

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
- **Scrolling**: the wheel scrolls the move history (`H`) when it is open, and
  does nothing on the board screen — read and deliberately discarded there,
  never typed as a keystroke. The game runs on the alternate screen, so your
  shell's scrollback is untouched and returns intact when you quit.

### Save and Load

There is no autosave and no "resume last game": a game exists only for as
long as it is open, unless you explicitly save it. This is deliberate —
resuming from a background save could silently drop you back into a
*finished* game (checkmate, a draw) with no result screen, showing a board
that looks live but is not. Saving is always a conscious choice instead, so
what you load back is always exactly the in-progress game you asked for.

- **Saving**: `s` during a game writes it to a new file in the `games/`
  directory, named after the date and time it was first saved plus a short
  id (`games/2026-08-16_140503-a3f9c1.chess`), without ending the game.
  Saving the opening position (no moves played yet) is refused — there is
  nothing worth loading in it. Saving again later updates that same file
  rather than creating another one, so a game you keep saving as you play
  stays a single file; a game you load and continue saving does too.
- **Loading**: the main menu's Load Game is a scrollable list of every saved
  game, showing when each was saved, how many moves it has, and whose turn it
  is. Choosing one (arrow keys + Enter, or a click to highlight it first)
  loads it and continues play from exactly that position.
- **Quitting**: `q` during a game asks to save and quit, quit without saving,
  or cancel — the moment a game in progress is preserved, since there is no
  background autosave to fall back on.
- **Format**: a save is two lines of text — the starting position as FEN, then
  the moves played from it in coordinate notation (`e2e4 e7e5 g1f3 ...`).
  Roughly a hundred bytes for an average game, human-readable, and portable: it
  does not depend on the compiler, the machine's byte order, or how any
  internal type is laid out in memory, so a file written by one build loads on
  any other, and a position pasted in from another chess program loads too.
  Loading replays every move through the legal-move generator, so a
  hand-edited or corrupted file is refused with an explanation — not
  half-loaded, and never deleted — rather than producing a silently wrong game.
- **Settings**: `settings.txt` stores the glyph set and colour scheme chosen
  in the Settings screen. A write failure there does not block the change —
  it still applies for the session, and the game says it will not persist.

Both `settings.txt` and the `games/` directory live beside the binary, in the
directory the game is run from.

## Versioning

Console Chess declares its version once, in `VERSION` at the repository root. Everything that reports a version — `console-chess --version` and the title bar — derives from that one file; a build fails rather than compiling a binary with a missing or malformed version.

Save files carry no version of their own: the format (`src/app/save.c`) is a FEN line plus a move list, validated structurally on load rather than by a version tag. A file from the old binary format is recognised by its now-unmistakable magic bytes and refused with a message saying so; anything else is accepted or rejected on whether it parses and replays, regardless of which build wrote it.

### Cutting a release

1. Edit `VERSION` with the new version number.
2. Commit the change.
3. Tag the commit `vX.Y.Z`.

## Notes

- Ensure your terminal supports Unicode and that the Nerd Font is correctly configured to display the chess piece icons. The game measures how wide your terminal draws them at startup and sizes the board to match, so the columns line up either way; `--ascii` avoids the question entirely.
- macOS and Linux. Windows would need a console-API backend.
