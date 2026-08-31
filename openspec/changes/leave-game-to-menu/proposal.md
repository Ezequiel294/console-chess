## Why

There is no way out of a live game except out of the program. `q` opens a picker whose two real options both exit and restore the terminal, so a player who wants to start a different game, load a saved one, or change a setting has to quit and relaunch. Every other screen in the program goes back where it came from — a replay pops, the result screen returns to the menu — and the board is the one place that does not.

That was defensible when the picker was written: with no autosave, the moment worth interrupting was the one where an in-progress game is lost for good, and quitting was the only thing that could lose it. But the interruption is about *saving*, not about *exiting*: leaving a game to the menu discards it exactly as thoroughly as leaving it to the shell. The question is right; the destination is wrong.

## What Changes

- **`q` during a live game leaves the game rather than the program.** The picker keeps its shape and its question — save first, or don't — and both answers land on the main menu instead of exiting. It is retitled to say so: `Leave this game?`, with `Save and leave`, `Leave without saving`, and `Cancel`.
- **Exiting the program stays exactly where it already is**: the main menu's own `Quit` entry, and `Ctrl-C` from anywhere. Nothing new is added for it, and nothing about it changes. **BREAKING** for anyone with the muscle memory of `q`-then-arrow-then-Enter meaning "I am done for the evening": that now takes them to the menu, and `Quit` there is one more Enter.
- **The save offer is unchanged in every respect that matters.** It still appears only when there is a game worth keeping, still opens on `Cancel`, still names the file the same way, and still refuses to save an opening position with no moves played. A game with nothing worth saving still offers only the two options.
- **The main menu's "returning to the menu" promise becomes reachable from a live game**, which is the one screen it was never true of.

## Capabilities

### New Capabilities

None. This narrows an existing choice's destination rather than adding anything.

### Modified Capabilities

- `app-shell`: `Quitting offers to save` becomes a requirement about *leaving a game*, whose two non-cancel outcomes are the main menu rather than a restored terminal; `Commands replace prompting` names the live-game command as leaving rather than quitting; `Irreversible choices open on the safe option` keeps its rule but restates the quit scenario in the new wording; `Main menu`'s "returning to the menu" gains the live game as a case it covers.
- `game-persistence`: `Saving is explicit` currently lists "by choosing to save when quitting" as one of the three moments a save happens. That moment is now leaving a game, and its scenario changes with it. The set of moments does not grow.

## Impact

- `src/app/game.c` — the quit picker: its title, its labels, and what `quit_activate` returns for the two acting options. `save_and_quit` becomes a save-and-leave that returns `CMD_RESET` to the main menu instead of `CMD_QUIT`; the same for the no-save option and for the two-option (nothing to save) form.
- `src/app/game.h` / `src/app/mainmenu.h` — the game screen needs to construct the main-menu screen to reset to it, which it does not today; `game.c` including `mainmenu.h` is what `gameover.c` already does for its own Return to Menu, so the include graph gains nothing new in shape.
- `README.md` — the key-bindings table's `q` row, and the Save and Load section's "Quitting" bullet.
- `src/app/help.c` — the commands block lists `q  quit`.
- No change to `save.c`, to the save format, or to any file on disk: what is written and when is exactly what it is today.
