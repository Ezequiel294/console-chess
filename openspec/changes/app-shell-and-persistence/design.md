## Context

See `proposal.md` — Why. Both remaining problems — saving spelled as quitting, and everything crammed onto one screen — are consequences of the game having no shell around the board.

The foundations are already in place. The screen stack from `terminal-ui-foundation` holds the screens; FEN, SAN, and `unmake` from `chess-rules-engine` supply the format, the display notation, and undo. Most of this change is assembly.

## Goals / Non-Goals

**Goals:**

- The player never loses a game, and never thinks about saving.
- The player is never asked a question they did not initiate.
- Each screen is independent enough to build and land one at a time.

**Non-Goals:**

- Full PGN with tags, comments, and variations. Movetext only.
- Cloud sync, multiple profiles, network play.
- A settings system beyond the three options named. A general preferences framework for three booleans is not worth building.
- Game analysis, evaluation, or hints. No engine exists.
- Migrating old save files. See Migration Plan.

## Decisions

### Autosave, and manual saving becomes a convenience

Save after every completed move. Startup asks `Resume last game?` instead of offering a save/load decision tree.

This inverts the current model, where a prompt every five moves is the only defence against losing a game and answering it quits. With autosave, closing the terminal loses nothing, and the `q` key needs no data-loss warning — only a confirmation against a mis-key.

Writing ~100 bytes per move is not a performance concern. Written to a temporary file and renamed into place, so an interruption mid-write cannot corrupt the existing save.

Named slots stay, because "save this position to come back to" is a different want from "do not lose my game," and slots serve it.

### Movetext, not a position snapshot

The file holds the starting position plus the moves played, not the current position:

```
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
e2e4 e7e5 g1f3 b8c6 f1b5
```

The move list is the history view, the undo stack, and the repetition record. Storing only the final position would discard all three, and they would have to be reconstructed or stored separately.

Loading replays the moves through the same generator that produced them, so an illegal move in the file is caught at load rather than trusted. A hand-edited or corrupted file is rejected instead of producing a subtly wrong game.

Cost: loading is O(moves) rather than O(1). At a few hundred moves this is imperceptible.

### Validation rejects rather than repairs

A file that fails validation is refused with an explanation, and the current game is untouched. No partial load, no best-effort repair. The current `load_game` reads whatever bytes are present and produces a garbage board silently; that is the behaviour being eliminated.

### Settings are a flat overlay, not a screen tree

Three options — glyph set, auto-flip, colour scheme — in one overlay. A nested settings hierarchy for three booleans is the over-screening trap noted during exploration. Stored as a small text file next to the save.

A settings write failure does not block anything: the change applies for the session and the player is told it will not persist.

### The status bar replaces prompting entirely

Available commands are always visible, with unavailable ones shown dimmed rather than absent — so `u` for undo is visibly present but greyed at move one, instead of appearing to do nothing.

Commands that do not alter the position work on either side's turn.

### Undo unwinds, it does not snapshot

Built on `unmake`. The move list is the undo stack: undo pops and unmakes, redo re-applies. Making a new move truncates the redo tail.

Alternative considered: store a position snapshot per move. Simpler, and it duplicates state that the move list already holds — and any drift between the two would be a bug. The reason `chess-rules-engine` specified `unmake` as restoring rights and clocks rather than only pieces was to make this possible here.

### Resignation and draw offers extend `game-outcome`

They are ways a game ends, so they belong with the other ways a game ends. Giving them a capability of their own would duplicate the termination-reason requirement across two specs and leave the result screen consulting both.

### Screens land one at a time

Each screen is independent — its own file, its own two functions, no knowledge of any other. The order below is by dependency, and the change is usable partway through: main menu and result first, then history, then settings.

## Risks / Trade-offs

- **Autosave corrupting a save if interrupted mid-write** → Write to a temporary file and rename. Rename is atomic on the filesystems that matter.
- **A rejected save file leaving the player with nothing** → Rejection never deletes. The bad file stays on disk, is named in the message, and is human-readable, so it can be inspected or hand-fixed.
- **Replaying moves at load surfaces a generator bug as a corrupt-file error** → Distinguish the two in the message: a move that parses but is not legal reports the move number and the position, which points at the generator rather than the file.
- **Undo and autosave disagreeing after an undo** → Autosave after undo as well as after a move. Covered by a spec scenario.
- **Over-screening** → Settings is deliberately an overlay. The bar for a new screen is that it owns the whole viewport and has its own navigation.
- **Seven screens is the largest surface in the sequence** → Each is independent and separately landable; a stall in one blocks nothing else.
- **Threefold repetition needs the position history, which undo mutates** → The move list is the single source for both, so unwinding one unwinds the other.

## Migration Plan

`save.c` is rewritten. Old files are rejected with a version message — no converter.

This is the third breaking format change in the sequence, and by design. The format broke at `modularize-and-build` (struct layout), again at `chess-rules-engine` (no castling rights), and is replaced here. Writing converters for a format with a known death date would have meant writing two throwaway converters. A game in progress across any of these boundaries is lost; acceptable for a local two-player game, and after this change the format is stable and text-based, so it should not happen again.

Screen build order: main menu and result screen first (they close the navigation loop), then history, then settings, then resignation and draw offers.

Rollback is `git revert`. Save files written by this change would not load into the previous version, so a rollback loses in-progress games.

## Open Questions

- Whether the fifty-move rule and threefold repetition draw automatically or must be claimed, carried over from `chess-rules-engine`. Detection is identical either way; this decides only whether the result screen appears unprompted or a claim command appears in the status bar. Resolve when building the result screen.
- Where save files and settings live — beside the binary, or under the user's config directory. Deferrable, changes no requirement.
- Whether the history view should allow stepping the board back through the game visually, rather than only listing moves. A genuine feature rather than a detail; out of scope here, and worth its own change if wanted.
