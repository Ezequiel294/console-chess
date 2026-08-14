## Why

`main.c` is a single 954-line file where rendering, input parsing, chess rules, persistence, and turn management are interleaved. Every planned improvement (a real TUI, legal-move generation, mouse input, screens) needs somewhere to live, and there is nowhere to put it. A previous attempt to add these features to this structure produced roughly 1000 lines that "got out of hand" and was reverted wholesale at `2820a0b`.

This change creates the container. It is deliberately behavior-neutral so that the four changes after it land in a codebase that can hold them.

## What Changes

- Split `main.c` into modules along the lines the later changes need, keeping every function's logic intact for now:
  - `main.c` — startup, teardown, entry point only
  - `game.c/h` — turn flow, the game loop, `GameState`
  - `board.c/h` — board representation, `init_board`, `update_board`, coordinate helpers
  - `rules.c/h` — `is_valid_move` (moved verbatim; rewritten later in `chess-rules-engine`)
  - `display.c/h` — all current `wprintf` output (replaced later in `terminal-ui-foundation`)
  - `history.c/h` — captures and move-history linked lists
  - `save.c/h` — `save_game` / `load_game`
- Add a `Makefile` with per-object compilation, header dependency tracking, and `all` / `clean` / `run` / `debug` targets. Build with `-std=c17 -Wall -Wextra`, and fix any warnings this surfaces.
- Introduce a `GameState` struct holding the board, both capture lists, the history list, and the move counter. Pass it as `GameState *` instead of the current mix of by-value and by-reference parameters.
  - This fixes a real defect: `main` passes the capture and history list heads **by value** into `game_loop` (`main.c:111`), so the lists `main` frees at `main.c:113-115` are always the original `NULL` heads. Every captured piece and every history node currently leaks. Bundling state into one struct makes this bug unrepresentable rather than merely fixed.
- Remove two redundant fields from `Piece_t`:
  - `position[3]` — square `[i][j]` is always `"abcdefgh"[j]` and `'8'-i`. Deleting it also deletes `find_piece_coordinates`, which scans 64 squares to answer a question that is two subtractions.
  - `icon` — a pure function of `(type, color)`; replaced by a lookup used at render time. This also separates presentation from the model, which `chess-rules-engine` depends on.
- Bump the save file format version and reject files written by the previous layout. **BREAKING** — existing `game_save.bin` files will not load. The format is replaced properly in `app-shell-and-persistence`; this change only prevents silently reading a struct layout that no longer exists.
- Replace the unbounded recursion in `get_move` (`main.c:279`, which re-calls itself on every invalid move) with a loop.

## Capabilities

### New Capabilities

None. This change introduces no new observable behavior.

### Modified Capabilities

None. The game plays identically before and after: same rules, same prompts, same output, same save-every-five-moves flow. The only user-visible difference is that old save files are rejected instead of misread.

This change sets `skip_specs: true` in `.openspec.yaml`. It is a pure refactor plus build tooling, and specs describe behavior.

## Impact

- **Code**: `main.c` is decomposed into 7 modules with headers. No algorithm changes; `is_valid_move` in particular moves verbatim so the diff stays reviewable.
- **Build**: `gcc -o chess_game main.c` no longer works. `make` replaces it. README updated.
- **Data**: existing `game_save.bin` files stop loading (see above).
- **Downstream**: every subsequent change in this sequence depends on this one. `terminal-ui-foundation` replaces `display.c`, `chess-rules-engine` replaces `rules.c`, `app-shell-and-persistence` replaces `save.c`.
- **Risk**: low. Mechanical, and verifiable by playing a game before and after.
