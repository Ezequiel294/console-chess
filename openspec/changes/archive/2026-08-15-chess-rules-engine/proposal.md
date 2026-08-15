## Why

The game is not currently chess. It has no castling, no en passant, and no promotion — a pawn reaching the back rank stays a pawn, and since every forward move then targets row `-1`, the piece is permanently frozen. There is no concept of check: a king may move into attack, a pinned piece may abandon its king, and the game ends when a king is literally captured rather than at checkmate.

All of that traces to one missing function. `generate_legal_moves(position, from)` is the keystone: castling, en passant, and promotion are just moves it emits or withholds; checkmate is `in_check && no legal moves`; stalemate is `!in_check && no legal moves`; and the legal-move highlight in the next change is a loop over its output. Build the generator and the whole list falls out of it.

The board cannot express a legal chess position either. Castling and en passant are history-dependent — the arrangement of pieces alone does not say whether they are permitted.

## What Changes

**Position model (`position.c/h`)**

Replace `Piece_t board[8][8]` as the unit of game state with a `Position` carrying everything a legal position requires:

- the board
- `side_to_move` (currently inferred from `moves % 2`)
- `castling_rights` — 4 bits, `KQkq`
- `en_passant_target` — a square or none, valid for exactly one ply
- `halfmove_clock` — for the 50-move rule
- `fullmove_number`

This set is exactly FEN, which is the format `chess-notation` below serializes.

**Move generation (`movegen.c/h`)**

- A `Move` struct: from, to, moved piece, captured piece, promotion choice, and flags for castling / en passant / double pawn push.
- `make(pos, move)` and `unmake(pos, move)` — `unmake` restores castling rights, the en passant square, and the clocks, not just the pieces. Undo in `app-shell-and-persistence` comes almost free from this.
- `is_square_attacked(pos, square, by_side)` — does triple duty: the legality filter, check detection, and castling's "may not pass through an attacked square" rule.
- Two-layer generation:
  1. **Pseudo-legal** — where a piece could move ignoring king safety. The existing `is_valid_move` is roughly 80% of this and is the starting point.
  2. **Legality filter** — for each candidate, `make` on a scratch position and reject it if the mover's king is attacked. This single step delivers pins, check evasion, and "king cannot move into check" for every piece at once, with no per-piece special-casing.

**Special moves**

- **Castling** — both sides, both wings, with all five conditions enforced: rights intact, squares between empty, king not currently in check, king not passing through an attacked square, king not landing in check.
- **En passant** — target square set by a double pawn push, cleared after one ply, including the pinned-capture edge case where the capture would expose the king along a rank.
- **Promotion** — the generator emits four moves to the same square (Q/R/B/N). The `Game` screen pushes a `Promotion` overlay to ask which, using the screen stack from `terminal-ui-foundation`. This is the first real overlay and validates that mechanism.

**Game outcome (`outcome.c/h`)**

- Check, checkmate, and stalemate.
- Draws: 50-move rule, insufficient material, and threefold repetition via Zobrist position hashing.
- **BREAKING** — king capture stops being the win condition. `captured_king` and its handling are removed. Kings can no longer be captured at all, because moves that would allow it are no longer legal.

**Notation (`notation.c/h`)**

- FEN parse and emit — needed here so test positions can be written as one-line fixtures, and reused by the save format in `app-shell-and-persistence`.
- Coordinate move text (`e2e4`, `e7e8q`) and SAN output for the history view.

**Correctness (`tests/perft.c`, `make test`)**

Chess has a standard correctness test: count leaf nodes of the move tree to depth N. From the starting position the answers are fixed — 20, 400, 8902, 197281, 4865609. Matching them proves castling, en passant, promotion, and pin handling simultaneously, and a mismatch bisects straight to the bug. Additional fixtures: "Kiwipete" (castling and en passant torture), plus positions covering promotion-with-check and en-passant pins.

This is the difference between believing castling works and knowing it does, and it is only possible because the rules layer takes no I/O.

## Capabilities

### New Capabilities

- `chess-rules`: the position model, move representation, make/unmake, attack detection, pseudo-legal generation, the king-safety filter, and the three special moves.
- `game-outcome`: check, checkmate, stalemate, and the draw conditions.
- `chess-notation`: FEN parse/emit, coordinate move text, and SAN.

### Modified Capabilities

None. No existing specs — `modularize-and-build` declared `skip_specs: true`, and the capabilities in `terminal-ui-foundation` are unaffected.

## Impact

- **Code**: `rules.c` from `modularize-and-build` is replaced. `game.c` switches from `moves % 2` to `position.side_to_move`. The `Game` screen gains the promotion overlay and a check indicator.
- **Hard constraint**: this layer performs **no I/O** — no `printf`, not even for errors. It returns values. That constraint is what makes it testable, and testability is what makes checkmate trustworthy.
- **Data**: **BREAKING** — save files from `modularize-and-build` cannot represent castling rights or the en passant square and will not load. The FEN-based format lands in `app-shell-and-persistence`; this change writes and reads FEN directly in the interim.
- **Build**: adds a `test` target running perft and the fixture suite.
- **Independence**: this change touches no terminal code and could be built before `terminal-ui-foundation` if desired — only the promotion overlay depends on the screen stack.
- **Downstream**: `mouse-and-highlights` calls `generate_legal_moves` for the highlight dots. `app-shell-and-persistence` uses `unmake` for undo, FEN for saving, and SAN for the history screen.
