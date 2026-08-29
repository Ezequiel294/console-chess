## Context

See proposal.md — Why, and its Depends on note: `add-named-saves` lands first and owns everything about how a game reaches the disk. This design assumes what that change delivers — a saved game that says whether it is finished, and a finished one that records how it ended — and adds nothing to persistence itself.

What matters technically here is the shape of what already exists:

- **The board screen is one screen.** `src/app/game.c` is 1200 lines, of which the board, the panels, the layout cache, the status bar and the hint row are the bulk. A replay draws all of that identically; only what the keys mean differs.
- **The undo mechanism is already built and unused.** `core/history.c` has `history_pop_last`, `history_push_node`, `captures_pop_last`, `hash_history_pop_last`; `movegen.c` has `unmake`, and `Move` carries the state it displaces (castling rights, en passant square, halfmove clock) so unmake is exact. `GameState.p_redo_head` exists for the moves popped off. Nothing calls any of it (see move-undo's Status note).
- **Screens are a vtable plus a context pointer**, with the region passed in per frame; each screen keeps one static context (`g_game`, `g_gameover`), since the app owns at most one of each at a time.
- **`savedgames.c` hands a loaded `GameState` to a callback** in `mainmenu.c`, which decides what screen to push. That is the seam a replay is routed in at.

## Goals / Non-Goals

**Goals:**

- One board renderer serving both live play and replay, so the two cannot drift apart visually.
- Stepping built on the existing undo mechanism rather than a second, parallel one.
- No write of any kind from a replay.

**Non-Goals:**

- Anything about saving, naming, or listing games. That is `add-named-saves`.
- Branching from a replay into a new game ("play on from here"). The spec forbids originating a move; a variation tree is a different feature.
- Replaying an in-progress game. Ongoing saves open in live play, unchanged.
- Any change to how live play works. Every live-play scenario in the existing specs must still pass untouched.

## Decisions

### One game screen with a mode, not a second screen

`Game_t` gains `Game_mode_t mode` (`GAME_MODE_LIVE` / `GAME_MODE_REPLAY`), and `game.h` gains a second constructor:

```c
Screen *game_screen(GameState *state);     /* live, unchanged */
Screen *replay_screen(GameState *state);   /* replay */
```

Both fill the same static `g_game`. The replay takes no separate outcome argument: `add-named-saves` puts the game's result on `GameState`, read back from the file, so the state already carries it. `game_render` is untouched except for the status bar and hint row, which branch on the mode. `game_handle` gains an early branch: in replay mode it dispatches to a small `replay_handle` covering `F`, `u`, `r`, `←`, `→`, `H`, `?`, `q` and ignores everything else, never reaching the selection/typing/move code at all.

*Alternative — a separate `replay.c` screen.* Rejected: it would either duplicate ~400 lines of board rendering or force an awkward extraction of the renderer into a third module. A mode flag on the screen that already knows how to draw a board is smaller and keeps the two views identical by construction.

*Alternative — reuse `gameover.c`'s `draw_final_board`.* Rejected: that board deliberately has no labels, no selection, no cursor, and no minimum size. A replay wants the live board, panels and all.

**Guard rather than trust:** the refusal to originate a move is enforced by never entering the move code, not by a check inside it. Clicks and typed characters are dropped in `replay_handle` before any square is named.

### Stepping is undo/redo, and the arrows are the same call

`replay_step_back` and `replay_step_forward` are two functions; `u`/`←` and `r`/`→` are four keys that call them. Back: `history_pop_last` → `unmake` → `hash_history_pop_last` → `captures_pop_last` if the move captured → push the node onto `p_redo_head`. Forward: the exact inverse. This is the mechanism move-undo already specifies; nothing new is invented for it.

The captured-pieces list is the one place needing care: it is a separate list from the history, so stepping must consult `move.captured` to know whether to pop or push a capture, and which side's list to touch — the same rule `save_read` already applies when building the lists forward.

Bounds: back is unavailable when `p_history_head == NULL`, forward when `p_redo_head == NULL`. Both are drawn dimmed in the hint row rather than omitted, matching how `s` is drawn when there is nothing to save.

Up and down arrows do nothing in replay. They exist in live play to move the board cursor, which is a way to name a square for a move — an act a replay does not have.

### The whole game is history + redo

Because stepping moves nodes between `p_history_head` and `p_redo_head`, the two lists concatenated are always the whole game in order: the redo list is built head-first by successive undos, so its head is the earliest undone move and appending it after the history yields forward order. The history view therefore computes SAN over `history ++ redo` and marks index `length(history) - 1` as the current move — no move-count field to keep in sync, no second copy of the move list.

`history_view_screen` takes the mode explicitly — `history_view_screen(const GameState *state, int mark_current)` — rather than inferring it from `p_redo_head` being non-empty, which is false at the final position, exactly where the view must still not truncate.

### The result comes off the state, and never off the screen

At the final position the status line states `state->result`. Nothing in the replay path constructs `gameover_screen` or returns `CMD_REPLACE`: the requirement is that the replay never leaves the board, and the way to guarantee it is that the code to leave it is not there.

A replay opened from a file with no recorded result — an externally authored or hand-written file, which `add-named-saves` still permits — is not a finished game and does not open as a replay at all, so this case cannot arise from the list.

### Entering a replay from the list

`savedgames.c` already reads the file and hands a `GameState` to its `on_loaded` callback. The callback (in `mainmenu.c`) branches on `loaded.result.reason != OUTCOME_IN_PROGRESS` and pushes `replay_screen` instead of `game_screen`. The saved-games screen stays beneath, so `q` from a replay (`CMD_POP`) returns to the list — the natural place to be after reviewing one game, and the only screen a replay can be reached from.

Live play's `q` (the save/quit picker, which exits the program) is not reused: a replay has nothing to save and nothing to lose, so `q` pops without a prompt, exactly as `Esc` does elsewhere.

### Orientation and the handoff

`replay_screen` initialises `flipped = 0` and never sets `awaiting_handoff`. `F` toggles `flipped`. Nothing else touches it. `f` is a file letter used by the typed-square field in live play, but the typed-square field does not exist in replay; the command is still bound to `F` (shift) rather than `f` so that the two modes never disagree about what a lowercase file letter means, and to match `H` for history.

## Risks / Trade-offs

- **A mode flag inside a 1200-line screen invites live-play regressions.** → The replay path branches *before* any live-play code, and the shared code (render, layout, hit-testing) is untouched. The live-play test surface is unchanged; the specs' live-play scenarios are the regression check.
- **`p_redo_head` was never exercised — the undo mechanism is specified and implemented but has never run.** → It is the first thing built and tested, ahead of any UI, over the special moves (castling, en passant, promotion, capture) where an inexact unmake would show up. The `Move` struct already carries the displaced state, so the risk is in the list plumbing, not in the position.
- **This change is unusable until `add-named-saves` lands** — without it there are no finished games on disk to open. → Deliberate: the alternative was duplicating the save work across two changes and reconciling two deltas against the same requirements. Build them in order.
- **A replay of a game saved by an older build, or edited by hand, may have no result to state.** → Such a file is a game in progress by definition and opens in live play, so the replay never has to render a blank result.
