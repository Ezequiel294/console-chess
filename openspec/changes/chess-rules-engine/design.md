## Context

See `proposal.md` — Why. The current `is_valid_move` validates one move at a time against piece geometry alone. It has no notion of check, so it cannot answer the questions everything else needs: "may this piece legally move here?" and "does this side have any move at all?"

The decisive constraint on this change: **this layer performs no I/O**. Not printing, not reading, not even error messages. It takes a position and returns values. That is what makes it testable, and testability is the only reason checkmate can be trusted.

It is also, as a consequence, independent of `terminal-ui-foundation`. Only the promotion picker needs a screen.

## Goals / Non-Goals

**Goals:**

- One generator that every feature queries — highlights, checkmate, undo, and the save format all consume the same output.
- Correctness that is demonstrated by counting, not asserted by inspection.
- A position type that can be copied onto the stack cheaply, since the legality filter does it for every candidate move.

**Non-Goals:**

- Speed. This is a two-human game; a straightforward mailbox generator running perft(5) in a few seconds is entirely adequate. No bitboards.
- An engine, evaluation, or search. There is no computer opponent, now or planned.
- Full PGN with tags, comments, and variations. Movetext only; see `chess-notation`.
- Chess960 or any variant.
- Time controls.

## Decisions

### Two-layer generation with a copy-make filter

Generate pseudo-legal moves from piece geometry, then filter: apply each candidate to a scratch copy and reject it if the mover's king ends up attacked.

This is the decision that pays for everything. Pins, check evasion, "king may not move into check", and the en-passant-exposes-the-king edge case all fall out of one filter with no per-piece handling. Writing them as explicit rules instead is where hand-written chess code goes wrong.

Alternative considered: generate only legal moves directly by computing pin rays and check masks first. Faster, and considerably harder to get right. Rejected on the correctness-over-speed grounds above.

`Position` stays small enough to copy by value — a 64-byte board plus a handful of scalars, roughly 80 bytes. Copy-make is therefore a struct assignment, which is why the position must not carry glyphs or square names. `modularize-and-build` removing those fields is the prerequisite.

### `is_square_attacked` does triple duty

One function answers three questions: is this move legal (does it leave my king attacked), is this side in check, and may the king castle across this square. Implemented by radiating outward from the square — sliders along their lines, knights at their offsets, pawns diagonally, king adjacent — rather than by generating all enemy moves and scanning them. Cheaper and simpler to verify.

### Mailbox board, a8 at index 0

The 8×8 array is kept, matching `modularize-and-build` and FEN's reading order. Off-board detection by explicit bounds checks rather than a padded 10×12 board — the padding trick saves comparisons this game does not need and costs readability.

### `unmake` restores state, not just pieces

An undo that restores the board but not castling rights or the en passant square produces positions that are subtly illegal, and the bug surfaces many moves later. Every `Move` therefore carries the state it displaced: captured piece, previous castling rights, previous en passant square, previous halfmove clock.

Alternative considered: keep a stack of full position copies and pop to undo. Simpler and impossible to get wrong, but the filter would copy a position per candidate per ply during perft. Rejected on that basis, though it remains the fallback if `unmake` proves error-prone — the perft numbers will say immediately.

This is also what makes undo in `app-shell-and-persistence` nearly free.

### Perft is the acceptance test

Counting leaf nodes to depth N from a known position produces a single number that must match a published constant. It checks castling, en passant, promotion, and pin handling simultaneously, and divided perft — per-move subtree counts — bisects a mismatch to the offending move in a few steps.

Fixtures: the initial position through depth 5, "Kiwipete" for castling and en passant, and the standard positions covering promotion-with-check and the en-passant pin. Depth 5 from the initial position is ~4.9M nodes, a few seconds without optimization, which is acceptable for `make test`.

Nothing else in this change is testable to remotely the same standard, which is why the no-I/O constraint is worth its cost.

### Zobrist hashing for repetition

Threefold repetition needs positions compared including side to move, castling rights, and en passant. Comparing full positions pairwise across the game history is O(n²) and fiddly. A Zobrist hash reduces it to comparing 64-bit keys, incrementally updated by make/unmake.

Collisions are theoretically possible and practically irrelevant at these depths. Keys are seeded from a fixed table, not from `rand()`, so hashes are reproducible across runs — which matters for tests.

### Promotion is four moves, not a move plus a question

The generator emits four distinct moves to the promotion square. The UI picks among them via an overlay. The alternative — one move plus a callback asking what to promote to — puts I/O inside generation, breaks the no-I/O constraint, and makes perft impossible, since perft must count all four.

### FEN lands here, not in the persistence change

`app-shell-and-persistence` is the consumer, but the tests in *this* change need to express positions as one-line fixtures. Building FEN later would mean writing a throwaway fixture format now.

## Inherited from `terminal-ui-foundation` as built

This layer stays independent of the terminal, so almost nothing carries over.
Two things do:

- **The promotion overlay has what it needs.** The screen stack ships with an
  `opaque` flag and `CMD_PUSH`/`CMD_POP`, screens are handed their region every
  frame, and only the top screen receives input — so a promotion picker composites
  over the board and returns to it with the board's state intact.
- **The call sites this change replaces are in one place.** `is_valid_move`,
  `update_board`, `update_captures` and `update_history` are now reached only
  from `submit()` and `finish_move()` in `src/app/game.c`, and the game-over test
  is the king-capture check in `finish_move()`. Deleting `rules.c` (task 5.6) and
  swapping `GameState` for `Position` touches those two functions and nothing
  else in the UI.

Note that the Game screen keeps its own selection and last-move state separately
from the position, which is the separation task 3.1 of `mouse-and-highlights`
also depends on: nothing about the interface leaks into what this layer models.

## Risks / Trade-offs

- **Subtle generation bug reaching production** → Perft. This risk is the entire justification for the no-I/O constraint.
- **`unmake` restores incorrectly, corrupting state deep in a search** → Perft catches it immediately, since a corrupted position produces wrong counts. If it proves fragile, fall back to copy-and-restore.
- **En passant pin case missed** → It is the classic omission. It has a dedicated spec scenario and a dedicated perft fixture.
- **Castling rights not revoked on rook capture** → Also classic, also covered by Kiwipete.
- **Perft(5) makes `make test` slow enough to skip** → Default `make test` runs to depth 4 (~200k nodes, well under a second); depth 5 and above behind `make test-full`.
- **Removing king capture changes how the game ends, and old saves encode games that reached illegal positions** → Accepted; see Migration Plan.
- **Zobrist collision producing a false draw** → Negligible at 64 bits over a few hundred positions. Not mitigated.

## Migration Plan

`rules.c` from `modularize-and-build` is replaced. `game.c` switches from `moves % 2` to `position.side_to_move` and from `captured_king` to the outcome type.

Save files cannot be migrated: the format has no field for castling rights or the en passant square, so a loaded game would silently permit or forbid castling incorrectly. Files are rejected with a version message, as in `modularize-and-build`. In the interim this change reads and writes FEN directly; the full format arrives in `app-shell-and-persistence`.

Build order within the change matters: `Position` and `make`/`unmake` first, then `is_square_attacked`, then generation, then perft. Perft is only meaningful once all four exist, so the first useful signal arrives late — expect the majority of debugging to happen after the first perft run rather than before it.

## Open Questions

- Whether the fifty-move rule and threefold repetition draw automatically or must be claimed. Chess requires a claim for both; automatic is friendlier for a casual game. Deferrable: the detection is identical either way, and the choice is a line in the screen that consumes it, decided in `app-shell-and-persistence`.
- Whether to keep a move-list capacity of 256 (safely above the ~218 maximum for a legal position) or size it dynamically. Deferrable, internal, changes no requirement.
