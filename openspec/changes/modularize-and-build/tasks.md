## 1. Build system

- [x] 1.1 Write a `Makefile` with per-object compilation into `build/`, `-std=c17 -Wall -Wextra`, and targets `all`, `clean`, `run`, `debug`
- [x] 1.2 Add header dependency tracking (`-MMD -MP`, include the generated `.d` files) so editing a header rebuilds its dependents
- [x] 1.3 Confirm `make` produces a binary that plays identically to the current one; keep a copy of the pre-change binary for comparison
- [x] 1.4 Update `README.md`: `make` replaces `gcc -o chess_game main.c`
- [x] 1.5 Add `build/` and the binary to `.gitignore`

## 2. Shared types

- [x] 2.1 Create `types.h` holding `Color`, `Piece_type_t`, and `Piece_t`, with an include guard
- [x] 2.2 Establish the header rule: no header includes a sibling header; only `types.h` is shared

## 3. Mechanical split

Move code without editing it. One commit per module keeps the diff reviewable.

- [x] 3.1 Extract `board.c/h` — `init_board`, `update_board`, `find_piece_coordinates`
- [x] 3.2 Extract `rules.c/h` — `is_valid_move`, moved verbatim including the `DEBUG` block
- [x] 3.3 Extract `display.c/h` — `print_board_white`, `print_board_black`, `print_history`, `print_captures`
- [x] 3.4 Extract `history.c/h` — `update_captures`, `update_history`, `free_captures`, `free_history`
- [x] 3.5 Extract `save.c/h` — `save_game`, `load_game`
- [x] 3.6 Extract `game.c/h` — `game_loop`, `get_move`
- [x] 3.7 Reduce `main.c` to the menu, initialization, and teardown
- [x] 3.8 Play a full game and diff the output against the pre-change binary

## 4. GameState

- [x] 4.1 Define `GameState` in `types.h`: board, both capture list heads, history head, move counter
- [x] 4.2 Change `game_loop` and `get_move` to take `GameState *`
- [x] 4.3 Change `save_game` / `load_game` to take `GameState *` / `const GameState *`
- [x] 4.4 Move the list heads out of `main`'s locals and into the `GameState` it owns
- [x] 4.5 Verify the leak is closed: capture several pieces, quit, and confirm under a leak checker (`leaks` on macOS, or `valgrind`) that the capture and history lists are freed

## 5. Remove redundant fields

- [x] 5.1 Add `square_to_index(const char *pos, int *i, int *j)` and `index_to_square(int i, int j, char out[3])` in `board.c`
- [x] 5.2 Replace every `find_piece_coordinates` call site with `square_to_index`, then delete the function
- [x] 5.3 Remove `position[3]` from `Piece_t`
- [x] 5.4 Add `piece_glyph(Piece_type_t, Color)` in `display.c`, backed by a lookup table
- [x] 5.5 Replace every read of `.icon` with `piece_glyph(...)`, then remove `icon` from `Piece_t`
- [x] 5.6 Simplify `update_board` now that it copies only type and color

## 6. Save format guard

- [x] 6.1 Write a 4-byte magic and a `uint32_t` version at the head of the save file
- [x] 6.2 Reject a file whose magic or version does not match, with the message `Save file was written by an older version and cannot be loaded.`
- [x] 6.3 Check every `fread` return value and reject a truncated file rather than proceeding
- [x] 6.4 Verify a pre-change `game_save.bin` is rejected cleanly instead of misread

## 7. Loop fix and cleanup

- [x] 7.1 Replace the self-recursion in `get_move` (`main.c:279`) with a loop
- [x] 7.2 Fix every warning `-Wall -Wextra` reports; for any that cannot be fixed without changing behavior, add a comment naming the change that will fix it
- [x] 7.3 Play a full game one more time against the pre-change binary, including save and load, and confirm identical behavior
