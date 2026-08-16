## Why

Playing requires typing four coordinates per move at two separate prompts, with no indication of what any piece can actually do. Clicking a piece and clicking its destination is how everyone expects to play chess, and showing the legal destinations turns the rules engine from something the game enforces silently into something the player can see.

This is the payoff change. It is small precisely because the two changes before it did the hard work: the input pipeline already parses mouse reports, the renderer already knows where the board is drawn, and `generate_legal_moves` already answers "where can this piece go."

## What Changes

**Click to square (`board_geometry`)**

The renderer knows the board's origin and cell size, so hit-testing is arithmetic:

```
\033[<0;24;7M   →   file = (24 - board_x) / cell_w
                    rank = ( 7 - board_y) / cell_h    (flipped for Black's view)
```

Clicks outside the board resolve to no square rather than to a wrong one. Cell width comes from the glyph-width probe in `terminal-runtime`, so this stays correct on terminals that render the Nerd Font icons double-width.

**Selection state machine**

```
IDLE ──click own piece──▶ PIECE_SELECTED ──click legal target──▶ move made ──▶ IDLE
  ▲                            │  │
  │                            │  └──click another own piece──▶ reselect
  └────click empty / ESC───────┘     click illegal square ──▶ flash, stay selected
```

**Mouse and keyboard converge on the same event.** A click, a typed `f6`, and arrow-keys-plus-Enter all produce `SelectSquare(f6)`; the state machine cannot tell them apart. The keyboard path is kept deliberately — a mouse-only game is unplayable over SSH.

**Visual affordances**

- **Legal-move dots** — a centered marker on every empty square in `generate_legal_moves(pos, selected)`.
- **Capture targets** — tinted background rather than a dot, so a capture reads differently at a glance.
- **Selected square** — distinct tint.
- **Last move** — from and to squares stay tinted until the next move. This is what replaced `sleep(1)` in `terminal-ui-foundation`; here it becomes visible alongside everything else.
- **King in check** — the king's square is marked whenever `in_check` is true.
- **Castling** — clicking the king shows the castling destination as a legal target, so it needs no special input syntax.
- Colors come from a small palette with a monochrome fallback, so the affordances survive a terminal without 256-color support.

**Scroll wheel**

Wheel events (buttons 64/65) are consumed and ignored on the game screen. They were the source of the injected junk in the previous attempt; the history screen in `app-shell-and-persistence` turns the same events into scrolling.

## Capabilities

### New Capabilities

- `board-interaction`: click-to-square hit testing, the selection state machine, convergence of mouse/typed/keyboard input onto one selection event, and the game screen's wheel-event policy.
- `move-affordances`: the visual layer — per-square tinting, legal-move dots, capture marks, selected-square and last-move highlights, the check indicator, and the monochrome fallback.

### Modified Capabilities

None. `input-events` and `screen-rendering` already contract to deliver mouse events and to draw into clipped regions; this change adds consumers of both rather than changing either. Requirements about what the board does with a wheel event belong to `board-interaction`, and requirements about square tinting belong to `move-affordances`.

## Impact

- **Code**: new `interaction.c/h` for hit testing and the selection machine. The `Game` screen's `handle` grows the state machine; its `render` grows the affordance pass.
- **Behavior**: typing coordinates still works. Nothing is removed — clicking is added alongside it.
- **Terminal caveat**: with mouse tracking enabled, click-drag text selection is unavailable in most terminals. Worth a README note, along with the usual `Shift`-drag escape hatch that most terminals provide.
- **Risk**: low. Both hard dependencies are in place, and the remaining work is arithmetic plus drawing.
- **Prerequisites**: needs `terminal-ui-foundation` (input pipeline, render geometry) and `chess-rules-engine` (the move generator). The only change in this sequence that requires two others.
