## Why

Two loose ends remain, and they turn out to be the same problem: the game has no shell around it.

**Saving is spelled "quit."** Every five moves the game interrupts to ask, and answering yes calls `exit(0)` (`main.c:174`) — there is no way to save and keep playing. The interrogation arrives when you did not want it, and the one thing you did want is unavailable.

**Everything lives on one screen.** Menus, history, help, and settings have nowhere to go, so the move history only appears after the game ends and the instructions only appear before it starts.

The save format also needs replacing. `fwrite(board, sizeof(Piece_t), 64, file)` writes struct padding, `wchar_t` (4 bytes here, 2 elsewhere), enum sizes, and native endianness straight to disk — a file that loads only on the machine and compiler that wrote it. `load_game` validates nothing, so a truncated file produces a garbage board silently.

## What Changes

**Save format**

Replace the binary struct dump with FEN plus movetext, using the `chess-notation` capability:

```
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
e2e4 e7e5 g1f3 b8c6 f1b5
```

Roughly 100 bytes, human-readable, portable, diffable — and positions can be pasted in from lichess, which makes every tricky castling or en-passant case a one-line fixture. Parsing is validated: a malformed or truncated file is reported, not loaded. **BREAKING** — `game_save.bin` is replaced by a text file and old saves do not migrate.

**Autosave**

Save after every move. Startup becomes `Resume last game? [Y/n]` rather than a save/load menu decision. Manual save slots become a convenience instead of the only defense against losing a game. The every-five-moves prompt is deleted outright.

**Screens** (using the stack from `terminal-ui-foundation`)

- `MainMenu` — New Game / Resume / How to Play / Settings / Quit
- `History` — the full move list in SAN, scrollable. This is where the wheel events that were injecting junk in the previous attempt become a feature: same parser, same events, different consumer.
- `Help` — rules of the interface, key bindings, mouse usage
- `Settings` — ASCII fallback, auto-flip on/off, color palette
- `GameOver` — result, reason (checkmate / stalemate / 50-move / repetition / insufficient material / resignation), and the final position
- Overlays: `ConfirmQuit`, `SaveSlot`, and a transient error toast

Settings deliberately stays a single flat overlay-style screen rather than a nested menu tree. Screens are cheap, not free; each one is UI to design and maintain.

**Commands**

Single-key bindings shown in a persistent status bar, replacing all prompting:

| key | action |
|---|---|
| `u` | undo — nearly free, `unmake` already exists in `chess-rules-engine` |
| `s` | save to a slot |
| `h` | history screen |
| `f` | flip board orientation |
| `?` | help overlay |
| `q` | quit (confirm overlay; autosave means nothing is lost) |

**Resignation and draw offers** — `chess-rules-engine` covers the draws the rules force. This adds the two a player chooses: resign, and offer/accept a draw.

## Capabilities

### New Capabilities

- `game-persistence`: the FEN + movetext format, validated parsing, autosave, resume-on-launch, and manual save slots.
- `app-shell`: the screen inventory above and navigation between them, plus the status bar and key-binding dispatch.
- `move-history-view`: the scrollable SAN history screen, including wheel-event scrolling.
- `move-undo`: undo and redo built on `unmake`, including how they interact with autosave and the history list.

### Modified Capabilities

- `screen-navigation`: `terminal-ui-foundation` shipped the stack with the game as the initial screen; this makes the main menu the entry point.
- `game-outcome`: gains two player-chosen terminations — resignation and agreed draw — alongside the rules-forced ones.

Resignation and draw offers are specified as a delta on `game-outcome` rather than as a capability of their own: they are ways a game ends, and splitting them out would duplicate the termination-reason requirement across two specs.

## Impact

- **Code**: `save.c` from `modularize-and-build` is rewritten. New screen modules, one per screen. `main.c` pushes `MainMenu` instead of `Game`.
- **Data**: **BREAKING** — `game_save.bin` is replaced. Given the format was unloadable across the two prior breaking changes anyway, no migration is written.
- **Behavior**: the every-five-moves prompt and the save-then-`exit(0)` path are both removed.
- **Risk**: low. Mostly plumbing over foundations that already exist. The largest single piece is the number of screens, and each is independent — they can land one at a time.
- **Prerequisites**: `terminal-ui-foundation` (screen stack) and `chess-rules-engine` (FEN, SAN, `unmake`, outcome reasons). Independent of `mouse-and-highlights`; the two could be built in either order.
