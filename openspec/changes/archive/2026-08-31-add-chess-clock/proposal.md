## Why

Chess is a timed game and this one is not. The space for a clock has been reserved from the start: the board sits in the left portion of the screen with the right side largely empty, and the SPACE handover between turns was written and documented as "the 'I'm ready' signal a future timed mode will need". This is that mode — and it turns the handover into what it was always mimicking, the press of a real clock button.

It also fixes the thing standing in the way: `New Game` currently starts a game with no chance to say what kind. A game now has settings, so it needs a screen that asks — one built to hold the questions that come after this change (opponent: another human or a bot) rather than one that has to be replaced when they arrive.

## What Changes

- **A chess clock beside the board.** Two rectangles in the empty column to the right of the existing captures/moves panel, stacked with a one-row gap, together spanning the board's height. White's is a light background with dark digits, Black's is dark with light digits, so which is which never depends on position. They ride the board's orientation: the bottom rectangle always belongs to the side the board currently faces, exactly as that side's pieces do. Untimed games get no clock column at all and look exactly as they do today.
- **SPACE is the clock press.** The player who just moved keeps burning their own time until SPACE is pressed; SPACE then applies their increment, starts the opponent, and flips the board — one gesture, one moment, everything at once. The handover stays in untimed games too: it is also what lets a player see their move before the board turns.
- **Time runs, and running out ends the game.** The clock ticks in real time, including while an overlay is open — help, the move list, a promotion picker and a save prompt do not stop your clock any more than walking away from a real board stops it. Reaching zero ends the game as a loss on time, unless the opponent has no material that could ever mate, which is a draw. A new termination reason, alongside checkmate and resignation.
- **A new-game setup screen.** `New Game` — from the main menu and from the result screen — opens a settings screen before the board: a `Time` row and an `Increment` row, each cycling through presets with the left and right arrows, and a `Start game` action. Up and down move between rows, Enter starts. Adding an `Opponent: Human / Bot` row later is one more row and nothing else moves.
- **Saved games carry their clock.** The save format's keyed trailer gains the time control, the time each side had left after every move played, and the time each side has left right now. Loading a timed game resumes with both clocks where they were, mid-turn included. A game saved with no time control loads untimed, and every save file already on disk keeps loading exactly as it does today.
- **Replaying a timed game shows the clock of the move you are on.** Stepping backward or forward moves the clocks with the board, the captures, and the check marker — the per-move times in the file are read back rather than recomputed, so the replay shows what the players actually saw.
- **The application can be woken by time, not only by input.** Today the program blocks until a key arrives; a clock that only advances when someone types is not a clock. The input wait gains a deadline and screens gain a notification that time has passed.

## Capabilities

### New Capabilities
- `chess-clock`: the clock itself — what a time control is, when each side's clock runs and stops, what the clock press does, increment, running out of time, and how the two clocks are drawn beside the board.
- `new-game-setup`: the screen between choosing New Game and the board — what it offers, how it is driven, what it does with what was chosen, and how it holds settings this change does not add.

### Modified Capabilities
- `game-outcome`: loss on time joins the list of ways a game ends, together with the insufficient-material exception that makes it a draw instead.
- `game-persistence`: the save format records the time control, the per-move clock readings, and the live clock; loading restores all three, and a file without them is an untimed game.
- `game-replay`: the clocks are among the things a step updates, read from the file's per-move readings.
- `app-shell`: New Game opens the setup screen rather than a board; the turn handover is stated as the clock press; and the commands row reflects that a live game can now end without anyone pressing a key.
- `input-events`: the wait for input gains a deadline, so time passing is something the program can act on without a keystroke.
- `screen-navigation`: screens can be told that time has passed, and — unlike input — that notification reaches screens beneath an overlay, because time passes for a game that is merely covered.

## Impact

- `src/core/` — a new clock module holding the time control and both remaining times, doing its arithmetic against a "now" it is handed rather than one it reads, so it stays as testable and as I/O-free as the rest of `core/`.
- `src/types.h` — `GameState` gains the clock; `History_node_t` gains the two remaining times as of that move (the per-move timestamp a replay needs).
- `src/ui/term.c` / `term.h` — a monotonic millisecond source, beside the existing bounded `term_read`.
- `src/ui/input.c` — a bounded `input_next`, careful not to truncate an escape sequence already in progress.
- `src/app/app.c` / `app.h` — the loop wakes on a deadline as well as on input, and notifies screens that time has passed.
- `src/ui/layout.c` — a clock region, taken from the width left over beside the panel.
- `src/app/game.c` — drawing the two rectangles, charging time, the clock press on SPACE, flag detection.
- `src/app/newgame.c` (new) — the setup screen; `mainmenu.c` and `gameover.c` route New Game through it.
- `src/app/save.c` — three new trailer keys, written and validated with the rest.
- `src/core/outcome.h` — the new termination reason and its draw variant.
- `src/app/help.c`, `README.md` — the clock, the clock press, and the setup screen.
- `tests/` — a suite for the clock's arithmetic, and save round-trip coverage for the new trailer keys.
