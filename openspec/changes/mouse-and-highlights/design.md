## Context

See `proposal.md` — Why. This change is small because its two prerequisites did the work: `terminal-ui-foundation` already parses mouse reports into typed events and already knows the board's drawn geometry, and `chess-rules-engine` already answers "where can this piece go."

What remains is hit-testing arithmetic, a three-state machine, and a drawing pass.

## Goals / Non-Goals

**Goals:**

- Pointing at a move is as expressive as typing it, and no more authoritative.
- What is marked as available is exactly what is legal — no approximation.
- The keyboard path survives untouched.

**Non-Goals:**

- Drag-and-drop. Click-click only; see Decisions.
- Right-click annotations or arrows.
- Hover previews. Requires motion tracking, which is a large increase in event volume for a small gain.
- Animation of moving pieces.
- Any change to move legality. This change only displays what the rules already decided.

## Decisions

### Click-click, not drag-and-drop

Drag needs motion tracking (`?1002` or `?1003`), which multiplies event volume, complicates the parser's state, and behaves inconsistently across terminals. Click-click needs only press events, maps identically onto the keyboard path, and is what chess sites offer alongside dragging anyway.

Revisit only if click-click proves unsatisfying in practice.

### Hit-testing derives from layout, never from constants

The board's origin and cell size come from the layout the renderer computed for the current frame, not from hardcoded offsets. Cell width in particular depends on the glyph-width probe from `terminal-runtime` — hardcoding it breaks on exactly the terminals the probe exists to handle.

A click outside the board resolves to nothing rather than clamping to the nearest square. Clamping means a click on the side panel silently moves a piece.

### One selection event, three producers

```
   click ─────┐
   typed "f6" ─┼──▶ SelectSquare(f6) ──▶ state machine
   cursor+Enter ┘
```

The state machine cannot tell which produced the event, and must not be able to. This is what keeps the keyboard path from decaying into a second-class path that quietly breaks — a real risk given that the game must stay playable over SSH.

### Selection state lives in the game screen

Three states — nothing selected, piece selected, awaiting promotion — held in the Game screen's own context. Not in `Position`: selection is interface state, and putting it in the position would break the no-I/O separation that `chess-rules-engine` depends on, and would leak into save files.

### Legal destinations are queried, never cached

On each frame with a piece selected, ask the generator for that piece's legal moves. Roughly 30 moves for a queen, filtered by a copy-make each — trivial at human speed, and it makes a stale-cache bug impossible. If it ever mattered, cache on selection change rather than on frame.

The set drawn is *exactly* the generator's output. Any divergence would mean a square that looks available but is refused, which is worse than no marking at all.

### Marks are background tint plus a glyph, not glyph substitution

Marking by replacing the square's content would hide the piece standing there — unusable for capture targets, which are precisely the squares with pieces on them. Instead: background tint for the square, plus a centered mark on empty destinations.

| state | empty square | occupied square |
|---|---|---|
| legal destination | centered dot | tinted background |
| selected | tinted background | tinted background |
| last move | tinted background | tinted background |
| king in check | — | distinct tint plus corner mark |

Tints come from a small palette. On terminals without colour, each state falls back to a distinct character in the square's corner, which is why the spec requires distinguishability by shape and not only by colour.

### The wheel is explicitly consumed

The game screen receives wheel events and discards them deliberately, with a comment. They were the source of the injected junk in the previous attempt. `move-history-view` in `app-shell-and-persistence` will consume the same events as scrolling — same parser, different consumer, which is the sign the layering is right.

## Risks / Trade-offs

- **Hit-testing drifts from what is drawn** → Derive both from one layout structure computed once per frame. Never compute board geometry in two places.
- **Off-by-one at square borders** → Test corners of every square, especially a1 and h8, at both glyph widths and both orientations.
- **Mouse tracking blocks text selection in the terminal** → Inherent to mouse reporting. Document the Shift-drag override most terminals provide.
- **Colour choices unreadable on some themes** → Require shape-based distinguishability, so colour is an enhancement rather than the mechanism.
- **The keyboard path silently rots** → It shares the event type with the mouse path, so a break in one is a break in both. Task 5 plays a full game without touching the mouse.
- **A terminal reports coordinates 0-based rather than 1-based** → SGR is specified 1-based; verify against a corner click on each terminal tested.

## Migration Plan

Purely additive. Typed coordinates keep working unchanged, so a player who ignores this change notices only the highlights.

New module `interaction.c/h` for hit-testing and the state machine; the Game screen's `handle` and `render` grow. Nothing is deleted.

Rollback is `git revert`; no persisted data is touched.

## Open Questions

- Whether clicking the selected square should cancel the selection or be ignored. Specified as cancel; if it proves annoying in play it is a one-line change affecting no other requirement.
- Whether to mark squares a selected piece defends but cannot legally move to, for example a defended friendly piece. Deferrable — additional information, no change to existing behaviour.
