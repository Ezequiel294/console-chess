## 0. Before starting

- [ ] 0.1 Confirm `add-named-saves` has landed: a finished game can be saved from the result screen, its filename says `finished`, and the file records the result. Nothing below works without it.

## 1. The stepping mechanism

The undo machinery in `core/history.c` and `movegen.c`'s `unmake` is implemented but has never run (see move-undo's Status note). It is built and proven first, before anything can be driven by it.

- [ ] 1.1 Add `replay_step_back` / `replay_step_forward` over a `GameState`: pop from `p_history_head`, `unmake` the move, `hash_history_pop_last`, pop the captured piece off the mover's capture list when `move.captured != FREE`, push the node onto `p_redo_head` — and the exact inverse forward. Place them where undo already lives (`core/history.c` plus the position call, or a thin `core/replay.c` if that keeps `history.c` list-only).
- [ ] 1.2 Add a `tests/test_replay.c` suite, declared in `tests/tests.h` and called from `tests/main.c` (the Makefile globs `tests/*.c`, so no build change is needed), asserting that stepping back then forward across a game restores the position bit for bit: FEN, castling rights, en passant square, both clocks, and the Zobrist hash.
- [ ] 1.3 Cover each special move in that suite: a capture, castling both wings, en passant, and a promotion — stepping back over each must restore the exact pre-move state, and the captured-pieces lists must match what they held before.
- [ ] 1.4 Cover the bounds: stepping back at the starting position and forward at the final position both leave the state untouched and report that there is nothing to do.
- [ ] 1.5 Assert that `history ++ redo` is the whole game in played order at every step position — the invariant the history view depends on.

## 2. The replay screen

- [ ] 2.1 Add `Game_mode_t` and a `mode` field to `Game_t`, plus `replay_screen(GameState *state)` in `src/app/game.h` / `game.c`, reading the result off the state. Live construction, `game_screen`, keeps its signature and behaviour.
- [ ] 2.2 Initialise a replay: `flipped = 0`, `awaiting_handoff` never set, no cursor, no selection, no typed square.
- [ ] 2.3 Branch `game_handle` on the mode before any live-play handling, dispatching to a `replay_handle` that recognises `F`, `u`, `r`, `←`, `→`, `H`, `?`, `q`, Ctrl-L and F5, and drops every other key, every click, and the wheel — so no code path that could originate a move is reachable.
- [ ] 2.4 Bind `u`/`←` to step back and `r`/`→` to step forward; up and down do nothing.
- [ ] 2.5 Bind `F` to toggle `flipped`, and confirm nothing else in the replay path writes to it.
- [ ] 2.6 Bind `q` to `CMD_POP`, with no quit picker — a replay has nothing to save.
- [ ] 2.7 Give the replay its own hint row: flip, step back, step forward, history, help, quit — with a step drawn dimmed when it would go past either end, the way `s` is dimmed when there is nothing to save.
- [ ] 2.8 Write the replay's status line: side to move for the current position, and — only at the final position — the result and its reason from the state. No `CMD_REPLACE` to `gameover_screen` from a replay under any circumstance.
- [ ] 2.9 Verify the captured panels, check indicator and last-move marking all follow a step, since they are drawn from state the stepping updates.

## 3. Getting into a replay

- [ ] 3.1 Branch `mainmenu.c`'s `on_game_loaded` on the loaded game's result: finished → push `replay_screen`, in progress → push `game_screen` as today, so the saved-games list stays beneath and `q` returns to it.
- [ ] 3.2 Confirm the saved-games list's own text still describes what choosing a game does now that finished games open differently from ongoing ones.

## 4. History in a replay

- [ ] 4.1 Change `history_view_screen` to take whether to mark the current move, and compute SAN over `history ++ redo` rather than the history alone.
- [ ] 4.2 Mark the move the board is currently showing — the last node of `p_history_head` — distinguishably, and mark nothing when the replay is stepped back to the starting position.
- [ ] 4.3 Open the view scrolled so the marked move is visible, rather than always at the bottom.
- [ ] 4.4 Confirm live play's history is unchanged: no marking, no moves beyond those played.

## 5. Help and the surrounding text

- [ ] 5.1 Add the replay's keys to `help.c`, and make the help shown from a replay describe the replay rather than live play.
- [ ] 5.2 Re-read every hint line and message touched by this change for a key that no longer exists or a mode it does not apply to.

## 6. Finishing up

- [ ] 6.1 Run the full test suite; every existing test must still pass untouched.
- [ ] 6.2 Drive the binary end to end: play a short game to checkmate, save it from the result screen, open it from Load Game, step both ways with all four keys, flip, open the history, and quit back to the list. (Per the project memory: drive it through a Python pty and reconstruct the frame — tmux is not installed.)
- [ ] 6.3 Repeat that check for a resigned game and an agreed draw, where the recorded result is the only record of how it ended.
- [ ] 6.4 Update `move-undo`'s Purpose in `openspec/specs/move-undo/spec.md` by hand: its "Status: deferred from live play" note now describes the past. A delta cannot carry a Purpose, so this does not happen at archive time.
- [ ] 6.5 Bump `VERSION`.
