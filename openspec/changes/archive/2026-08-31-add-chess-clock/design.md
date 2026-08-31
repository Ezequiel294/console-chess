## Context

See proposal.md — Why. The constraints that actually shape this design are in the existing code:

- **`src/core/` performs no I/O and touches no terminal**, which is what lets it be tested without one (`tests/` links `core/*.c` plus `app/save.c` and nothing else). A clock is the first thing in this program that cares what time it is.
- **The event loop blocks.** `app_run` calls `input_next()`, which waits indefinitely, and draws a frame only after an event arrives. Nothing in the program currently happens on its own.
- **Input goes to the top screen only.** `app.c` dispatches `handle` to `top()`. Rendering, by contrast, already walks down to the topmost opaque screen and draws upward, so covered screens are already drawn while not being driven.
- **A game screen already defers endings.** `apply_outcome` sets `game_over` and `pending_outcome`; the transition to the result screen happens on the *next* handled event, because several endings are discovered while an overlay is on top. Flag fall is exactly this shape.
- **The save file is a FEN line, a moves line, and a keyed trailer** of `key value` lines, where an unknown key is a hard rejection (`parse_trailer`). The trailer is the designed extension point and this change is its first real use.
- **`Layout` is recomputed every frame** from the terminal bounds and hands out `title / board / panel / status` rects. Minimum size is `board_block_w + 1 + PANEL_MIN_W` by `TITLE_H + board_block_h + STATUS_H` — 68x23 at a two-cell glyph.
- **The renderer diffs frames**: an identical frame writes nothing. A ten-times-a-second repaint of a screen whose digits have not changed costs one memcmp-ish pass and zero bytes to the terminal.

## Goals / Non-Goals

**Goals:**

- Clock arithmetic that is pure, total, and testable with no time source and no terminal — the same standard the rest of `core/` is held to.
- Time that advances correctly whether or not anyone types, and that cannot be gained or lost by opening an overlay, resizing, or the machine's wall clock moving.
- A save format extension that is legible in a text editor and that leaves every file already on disk loading unchanged.
- A setup screen whose structure absorbs a future `Opponent` row without being rewritten.

**Non-Goals:**

- Any opponent that is not a second person at the same keyboard. The setup screen is built to hold that row; this change does not add it.
- Time controls beyond "initial time plus a fixed increment per move" — no delay, no Bronstein, no multi-stage controls, no per-side asymmetry.
- Clock sound, low-time warnings, or any animation.
- Changing the minimum terminal size, or reflowing the board to make room.

## Decisions

### The clock is pure arithmetic that is handed the time

`src/core/chessclock.{c,h}` owns a `Chess_clock_t`: the time control (initial ms, increment ms), both sides' remaining ms, which side is running, and the monotonic timestamp that side started at. Every function takes `now_ms` as a parameter — `clock_start(c, side, now)`, `clock_press(c, now)`, `clock_remaining(c, side, now)`, `clock_expired_side(c, now)`. It reads no clock of its own.

*Why:* it keeps `core/` I/O-free and makes the whole of the clock's behaviour — increment, flag fall, mid-turn save and resume — testable by passing fabricated timestamps, with no sleeping and no flaky timing tests. The one thing that genuinely needs the operating system, "what time is it", becomes a single function elsewhere.

*Alternative rejected:* a clock that calls `clock_gettime` internally. Simpler to call, untestable without sleeping, and it would be the first I/O in `core/`.

**Untimed is `initial_ms == 0`,** not a separate flag. Everything reads `clock_is_timed()`; there is one representation of "no clock" rather than a boolean and a value that can disagree.

### The time source is `term_now_ms`, and it is monotonic

`uint64_t term_now_ms(void)` joins `term_read`'s bounded wait in `ui/term.c`, using `clock_gettime(CLOCK_MONOTONIC)`.

*Why term.c:* it is already the platform layer — termios, signals, the bounded read, the glyph-width probe. A separate module for one function is not worth an entry in the include graph.

*Why monotonic:* `CLOCK_REALTIME` moves when NTP corrects it or the user changes the clock, and a player would gain or lose the difference. `CLOCK_MONOTONIC` is available on both macOS and Linux, which is the supported set.

### Remaining time is computed, never accumulated

`clock_remaining` returns `base_remaining[side] - (now - started_at)` for the running side and `base_remaining[side]` for the other. Nothing is subtracted per tick.

*Why:* a tick that is late, early, missed, or delivered twice cannot corrupt the clock — the reading is a function of two timestamps, not a running total. This is what makes the tick purely a repaint-and-check concern rather than the mechanism time is kept by, and it is why "the terminal was too small for a while" is handled by *restarting the interval*, not by trying to subtract the gap.

### Waking up: a bounded `input_next`, plus a per-screen wake request

Three pieces:

1. **`input.c`** gains `Event_t input_next_within(int timeout_ms)`; `input_next()` becomes `input_next_within(-1)`. The deadline is a wall-clock budget over the whole decode loop, and the per-iteration wait handed to `term_read` is `min(pending_timeout(), remaining budget)`. A partial escape sequence keeps its bytes in `g_buf` across the expiry, exactly as it already does across `TERM_READ_INTR` — the buffer is untouched, so the sequence resumes rather than restarting. Expiry surfaces as a new `EV_TIMEOUT`.
2. **`Screen` gains two optional members**: `int (*wake_in_ms)(void *ctx)` — how soon this screen needs waking, or `-1` — and `void (*tick)(void *ctx, uint64_t now_ms)`. Both default to NULL, so no existing screen changes.
3. **`app_run`** takes the minimum `wake_in_ms` over *the whole stack*, waits that long, and calls `tick` on every screen that has one, top to bottom, before drawing. `EV_TIMEOUT` is swallowed by the loop and never dispatched to `handle`.

*Why tick reaches covered screens while input does not:* input is a statement about what the user did, and only the top screen may act on it. A tick is a statement about the world, and the world does not stop for a covered screen. The game's clock must run while the promotion picker is up; the alternative — the overlay forwarding time to the screen underneath — would require every overlay to know it might be covering a game.

*Why `tick` returns nothing:* letting a covered screen return a `Cmd_t` would mean a screen that is not on top moving the stack, which `screen-navigation` forbids for good reason. The game screen instead sets `game_over` / `pending_outcome` on flag fall — the path `apply_outcome` already uses — and the status line immediately reads the result plus "press any key to continue", exactly as it does for every other ending discovered under an overlay. `CMD_POP_REPLACE` stays what it is: something an *overlay* returns.

**Wake interval: 100 ms while a clock is running**, `-1` otherwise. That is fast enough for the tenths shown under ten seconds and slow enough to be free — and because the renderer writes nothing when the composed frame is identical, the ~590 wakeups of an untouched minute produce ten writes a second only in the last ten seconds of a clock.

### Flag fall is detected on the tick and only on the tick

`clock_expired_side` is consulted in the game screen's `tick`. It cannot be checked "when a move is made" alone — that is precisely the case where nobody is pressing anything.

The insufficient-material exception needs a per-side question that `outcome.c` does not currently answer: `outcome_can_mate(pos, side)` — false for a lone king, king+bishop, king+knight; true for everything else including king+two-knights, since a helpmate exists. This is deliberately *not* the existing `insufficient material` draw test, which is a both-sides question. Two new reasons: `OUTCOME_TIMEOUT` and `OUTCOME_DRAW_TIMEOUT_INSUFFICIENT_MATERIAL`, constructed by the app layer the way resignation already is (`outcome()` sees only a `Position` and cannot know about a clock).

### The clock column comes out of the width already there

`Layout` gains a `clock` rect. `layout_min_cols` / `layout_min_rows` are **unchanged**, so no terminal that plays today stops playing.

The board is what runs out of room first, and it runs out vertically: the minimum is 68x23 at a two-cell glyph, and terminal windows are far wider than they are tall, so shrinking one hits the 23-row floor with columns to spare. The clock therefore takes its column from whatever exceeds `PANEL_MIN_W` beside the panel: if `bounds.w - board_w - 1 >= PANEL_MIN_W + 1 + CLOCK_W` (with `CLOCK_W` 12), the clock gets its column at the right edge and the panel keeps the rest; otherwise `clock` comes back zero-width and the game screen draws both times as two lines at the top of the panel instead.

*Why keep the fallback at all,* given the reasoning above says it will not be reached: it is about fifteen lines, and the alternative is a layout with an undefined case. A timed game whose clock is invisible is the one outcome that must not be possible.

**Geometry:** the two rectangles span `layout.board.h` between them — `(h - GAP) / 2` each with a one-row gap, so they grow and shrink with the board and are guaranteed to fit vertically whenever the board does. Each is a box with the side's name on one row and the time on another, centred.

**Colours:** White is fg 232 on bg 254, Black is fg 252 on bg 236 — fixed, not part of the settings palette, because they are the *pieces'* colours rather than the board's tint scheme. On `term_supports_color() == 0` the backgrounds drop and the side's name carries the distinction, which the spec requires anyway.

**Orientation:** the lower rectangle belongs to `g->flipped ? BLACK : WHITE`. It follows the existing `flipped` field and needs no state of its own, which is what makes it correct in a replay's manual flip for free.

### Formatting: `h:mm:ss` / `m:ss` / `s.t`, always rounded down

`clock_format(ms, buf, len)` in `core/`, so it is unit-testable. Below 10 000 ms it is `%d.%d`; below an hour `%d:%02d`; otherwise `%d:%02d:%02d`. Truncation, not rounding, so a clock reading `0:01` really does have at least a second — the spec's "never rounded up".

### Per-move times live on the history node

`History_node_t` gains `int32_t remaining_ms[2]` — what each side had left *after* that move, increment included. It is written when the clock is pressed and read directly by the replay.

*Why on the node:* replay already moves nodes between `p_history_head` and `p_redo_head`, and `replay_step_back` / `_forward` already update the position, hashes and captures together. Carrying the clock reading on the node means the replay's clock follows the step by construction, with no parallel array to keep in sync and no index to get wrong. It is also exactly the "timestamp associated to each move" the request asked for, in the form a replay can use without recomputation.

*Alternative rejected:* storing an elapsed duration per move and summing. Equivalent information, but a replay would have to fold over the whole list to display one position, and any inconsistency in the file would silently distort every later reading rather than being caught where it is.

**The mover's own increment is included** in the reading stored for their move, since the reading is taken at the press, after the increment is applied. That is what a real clock shows.

### Three new trailer keys

```
timecontrol 600000 2000
clocks 597400,600000 597400,594100 591200,594100
remaining 588300,594100
```

- `timecontrol <initial_ms> <increment_ms>` — absent means untimed.
- `clocks` — one `white,black` pair per move played, in order, matching `remaining_ms[]` on each history node.
- `remaining <white_ms>,<black_ms>` — the live reading at the moment of writing, which differs from the last `clocks` pair whenever the game is saved mid-turn. For a finished game the two agree.

*Why milliseconds as integers:* the trailer is already `key value` text, integers parse with `strtol` and no locale or floating-point concerns, and the file stays diff-able. Legible enough — `600000` beside `timecontrol` reads as ten minutes without effort.

*Why all three rather than deriving:* `timecontrol` alone cannot give a mid-turn resume; `remaining` alone cannot give a replay; `clocks` alone cannot say what control was played, since a game may end before any increment is visible.

**Validation** (`parse_trailer`, which already rejects unknown and duplicate keys) additionally rejects: a `clocks` count that is not the move count, any value negative or exceeding `initial + increment * moves_by_that_side`, and `clocks`/`remaining` present without `timecontrol` or vice versa. Rejection is the existing `SAVE_READ_NOT_A_SAVE_FILE`, so nothing new appears in the read-status enum.

**Backward compatibility is free**: absent keys mean untimed, which is what every file on disk today is.

### The setup screen: rows of `Setting_t`, one array

`src/app/newgame.{c,h}`, opened by both `mainmenu.c` and `gameover.c` in place of their direct `game_screen` push, calling back with a completed `Chess_clock_t` (and, later, whatever else a game needs) — the same `on_game_loaded` callback shape `savedgames.c` already uses.

Its state is an array of rows plus a selected index. A row is a label, a list of option strings, a current index, and an `enabled` predicate; the last row is the `Start game` action. Up/down move `selected`, left/right move the highlighted row's index, Enter acts only on the action row. Adding `Opponent` later is one entry in the array.

*Why not the flat menu list every other screen uses:* time and increment are two independent settings, and a flat list of named controls (`Blitz 3+2`) makes them one. The row form is also the only one a future `Opponent` row joins without redesign. The gesture split — up/down navigates, left/right changes, Enter commits — is what keeps this consistent with `app-shell`'s "a click only selects, Enter is the one way to act".

Presets: time `Untimed, 1, 3, 5, 10, 15, 30, 60 min, Custom…`, default 10 min; increment `0, 1, 2, 3, 5, 10, 30 s, Custom…`, default 0. `Custom…` pushes the existing `prompt_screen` with a numeric validator — it already does titled text entry with a validator, a submit callback and Escape-to-cancel, so no new input screen is written. When time is `Untimed` the increment row draws dimmed and its left/right are ignored, matching how unavailable commands are already shown rather than hidden.

## Risks / Trade-offs

- **A 100 ms wake means the loop is no longer idle during a timed game.** → Reading is `now - started_at`, so the work per wake is one `clock_gettime`, one frame composition, and a diff that writes nothing when the digits have not changed. The wake request is `-1` — a true indefinite block — in menus, in replays, in untimed games, and whenever no clock is running.
- **Charging the mover until SPACE penalises a slow handoff.** → This is the chosen behaviour, not an accident: the handover *is* the clock press, so the time between moving and pressing is the mover's, exactly as it is on a real clock. It is worth stating in the help text so it is never a surprise.
- **`tick` reaching covered screens is a genuine widening of the screen contract.** → Narrowed by making it opt-in (NULL by default), by giving it no return value, and by forbidding it from moving the stack. Only the game screen implements it.
- **A flag fall under an overlay does not close the overlay.** → The game underneath records the ending and the result screen appears when the overlay is dismissed — identical to how a resignation decided under a confirmation already behaves. The alternative is a non-top screen mutating the stack.
- **A hand-edited save can claim more time than it could have had.** → Bounded by the `initial + increment * moves` check. It cannot be made airtight without recording every move's duration, and the format is explicitly hand-editable, so the check is a sanity bound rather than a guarantee.
- **`History_node_t` and `GameState` both grow.** → `Move` is copied per candidate in the legality filter, but `History_node_t` is not; `GameState` is always passed by address. Eight bytes on the history node and a `Chess_clock_t` on the game state are on no hot path.
- **`clocks` grows the file by roughly 14 bytes a move.** → A hundred-move game gains about 1.4 KB against a file currently around 100 bytes. Still trivial, still one screen of text, and it is what buys the replay.

## Migration Plan

No migration. Files on disk record no `timecontrol` and therefore load as untimed games, which is what they are; a save written by this change is rejected by an older binary in exactly the way that binary already rejects unknown trailer keys, which is the documented behaviour rather than a regression. `VERSION` gets a minor bump at release time under the existing procedure in README.md.

## Open Questions

- Whether the low-time state deserves any visual emphasis (the digits changing colour under ten seconds, say). Left out deliberately — it changes nothing in the specs, the layout, or the task breakdown, and is better judged against the thing once it is on screen.
