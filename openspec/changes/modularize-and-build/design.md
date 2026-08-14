## Context

`main.c` is 954 lines containing 17 functions covering rendering, input, rules, persistence, and turn management. See `proposal.md` — Why.

Two properties of the current code shape this design:

- **Presentation is embedded in the model.** `Piece_t` carries `icon` (a `wchar_t` glyph) and `position` (the square's own name as a string). Both are derivable, and both mean the rules layer cannot be separated from the display layer without dragging glyph data along.
- **State is scattered across parameters.** `game_loop` takes five arguments, three of which are linked-list heads passed by value while the functions below it take the same heads by address. That inconsistency is the leak described in the proposal.

The constraint on this change is that it must be **verifiable by inspection and by playing**. There is no test suite yet (the first one arrives with `chess-rules-engine`), so correctness here rests on the diff being mechanical enough to read.

## Goals / Non-Goals

**Goals:**

- A module boundary for every layer the next four changes need to replace, so each later change rewrites whole files rather than editing around foreign code.
- One owner for game state.
- A build that catches what `gcc -o chess_game main.c` currently does not.

**Non-Goals:**

- Improving any algorithm. `is_valid_move` moves verbatim, bugs included; it is rewritten in `chess-rules-engine`.
- Improving output. `display.c` is a holding pen for the existing `wprintf` calls, replaced wholesale by `terminal-ui-foundation`.
- Test infrastructure. Deferred to `chess-rules-engine`, which is the first layer that can be tested without a terminal.
- Save file compatibility. See Migration Plan.

## Decisions

### Module boundaries follow the replacement plan, not the current code

The obvious split is by what the code does today. The split chosen instead is by **what replaces each part later**:

| module | replaced by |
|---|---|
| `display.c` | `terminal-ui-foundation` (entirely) |
| `rules.c` | `chess-rules-engine` (entirely) |
| `save.c` | `app-shell-and-persistence` (entirely) |
| `board.c`, `history.c` | survive, adapted |
| `game.c` | becomes the Game screen in ② |

Alternative considered: split by data structure (`piece.c`, `list.c`, `board.c`). Rejected — it produces modules that every later change has to edit simultaneously, which is the coupling this sequence exists to avoid.

### `GameState` is a struct, not globals

Globals would fix the leak just as effectively and with a smaller diff. Rejected because `chess-rules-engine` requires a position it can copy onto the stack, mutate, and discard for the legality filter — that is impossible against global state. Paying for `GameState *` now avoids paying for it twice.

### `icon` and `position` are deleted, not deprecated

Keeping them and ignoring them would be a smaller change. Rejected: `position` is the only reason `find_piece_coordinates` exists, and leaving a 64-square linear scan in place invites later code to call it. Deleting the field deletes the function and forces the two-subtraction conversion everywhere.

`icon` moves to a lookup indexed by `(type, color)` in `display.c`. This is what makes the ASCII fallback in ② a table swap rather than a rewrite.

### Board orientation stays as-is

`board[0][0]` remains a8, matching the current code and FEN's reading order. Tempting to flip to a1-origin for rank arithmetic. Rejected — it would silently invert every existing comparison in `is_valid_move`, defeating "moves verbatim."

### Warnings are fixed, not suppressed

`-Wall -Wextra` on this code will surface unused parameters, sign comparisons, and likely the unchecked `fread` returns in `load_game`. Each gets fixed. Any that cannot be fixed without changing behavior gets a comment naming the change that will fix it.

## Risks / Trade-offs

- **A "mechanical" refactor silently changes behavior** → Play a full game against the pre-change binary and compare board output at each move. Keep the split and the `GameState` introduction as two separate commits so a bisect can tell them apart.
- **`-Wextra` surfaces a warning that is a real bug, tempting an in-scope fix** → Fix only if it is a crash or memory error. Otherwise record it in the proposal for the change that owns that code. Scope creep here delays every downstream change.
- **Header cycles** (`board.h` needing `Piece_t`, `game.h` needing both) → One `types.h` holding shared enums and structs; no other header includes a sibling. Include guards everywhere.
- **Removing `position` breaks the save format, which breaks in-progress games** → Accepted; see Migration Plan.
- **Seven modules for ~950 lines is over-decomposition** → Accepted deliberately. The line count roughly triples across this sequence, and the boundaries are chosen for where that growth lands.

## Migration Plan

The save format changes because `Piece_t` changes. Options considered:

1. **Convert old saves on first load** — rejected. The format breaks again in `chess-rules-engine` (no castling rights) and is replaced entirely in `app-shell-and-persistence`. Writing a converter twice for a format with a known death date is wasted work.
2. **Reject with a clear message** — chosen. `save.c` writes a 4-byte magic and a version. On mismatch: `Save file was written by an older version and cannot be loaded.` No silent misread, which is the current behavior.

There is no rollback concern — this is a local single-player game, and the previous binary remains in git.

## Open Questions

- Whether `history.c` keeps linked lists or moves to a growable array. Linked lists were an academic requirement that no longer applies, and an array indexes better for the history view in ⑤. Deferrable: the module boundary is the same either way, and `app-shell-and-persistence` can swap the internals without touching callers.
