## 1. The picker leaves the game rather than the program

- [ ] 1.1 Have `quit_activate` in `src/app/game.c` return `(Cmd_t){CMD_RESET, mainmenu_screen(g->state)}` for the "leave without saving" option in both the three-option and the two-option form, in place of `CMD_QUIT`. Verify by pressing `q` in a live game, choosing it, and landing on the main menu with the program still running.
- [ ] 1.2 Turn `save_and_quit` into a save-and-leave: the same `save_flow`, with `CMD_RESET` to the main menu as both the `after_direct` argument and the return of its name-prompt submit callback. Verify both paths — a game that already has a file saves and lands on the menu with no prompt, and one that does not is asked for a name first and lands there once it is given.
- [ ] 1.3 Confirm the name prompt's Escape still cancels only the prompt, leaving the picker up and the game untouched, rather than leaving the game unsaved.
- [ ] 1.4 Add `#include "app/mainmenu.h"` to `game.c` and verify the build is clean — `gameover.c` already includes it for the same reason, so no cycle is introduced.

## 2. Wording

- [ ] 2.1 Retitle the picker `Leave this game?` and relabel its options `Save and leave` / `Leave without saving` / `Cancel`, and `Leave` / `Cancel` in the two-option form. Verify each label by eye in both forms — with a move played, and on the opening position.
- [ ] 2.2 Verify the picker still opens with the highlight on `Cancel` in both forms, and that Enter without moving returns to the game unchanged (app-shell's Irreversible choices open on the safe option).
- [ ] 2.3 Change the game screen's status-bar hint from `q quit` to `q leave`, and the help screen's commands block likewise. Verify both read correctly, and that the help box still fits whole at 23 rows — it has no spare lines.
- [ ] 2.4 Update `README.md`: the key-bindings table's `q` row, the "Quitting" bullet in Save and Load, and any sentence that says quitting a game exits the program. Verify the table and that section both describe what the program does.

## 3. The game that was left

- [ ] 3.1 Verify leaving without saving discards the game: the main menu offers no way back to it, and starting a new game or loading a saved one begins from nothing the abandoned game left behind (no stale captures, move list, hash history, or result).
- [ ] 3.2 Verify leaving a *timed* game and then starting an untimed one draws no clock, and that leaving an untimed game and starting a timed one draws both — the clock lives on the `GameState`, which the next game overwrites wholesale.
- [ ] 3.3 Verify a game saved on the way out is byte-identical to what the in-play save command writes at that moment, clock included, by saving one of each from the same position and comparing the files.
- [ ] 3.4 Build with `make debug` (ASan/UBSan) and leave a game to the menu, then start a new one, then load a saved one, twice over. Verify no sanitiser reports and no leak of the abandoned game's lists.

## 4. Everything else that leaves a screen

- [ ] 4.1 Verify the main menu's own `Quit` still exits the program and restores the terminal, and that it is now the only screen offering that.
- [ ] 4.2 Verify `Ctrl-C` still exits immediately from a live game with the terminal restored, unchanged by this change.
- [ ] 4.3 Verify the result screen's Return to Menu and a replay's `q` are unaffected, and that a game reached through Load Game or through the new-game setup screen leaves to the main menu rather than back to the list or the setup screen it came from.
- [ ] 4.4 Run `make test` and `make test-full`; verify everything passes, since nothing in `core/` or `save.c` is touched.
