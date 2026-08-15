## Context

See `proposal.md` — Why, which names the three root causes of the failure that ended the previous attempt (alternate scroll mode, unhandled wheel reports, two readers on one descriptor).

Constraints shaping this design:

- **Hand-rolled ANSI, no ncurses.** Chosen deliberately; revisit only if this layer becomes unmaintainable. Everything below is therefore ours to get right, including the parts a library would have handled.
- **POSIX only.** `termios`, `ioctl`, `signal`. macOS and Linux.
- **A previous attempt failed here.** That is the dominant design input. Where a choice trades elegance for the impossibility of a past failure mode, take the latter.

## Goals / Non-Goals

**Goals:**

- Junk input is structurally impossible, not merely unlikely.
- The terminal is restored on every exit path, including a crash.
- Resize correctness is a property of the architecture, not something each screen implements.
- Later changes add screens without touching this layer.

**Non-Goals:**

- Mouse input handling. The parser emits mouse events here; the first consumer arrives in `mouse-and-highlights`.
- Menus, history, settings. Screens for those arrive in `app-shell-and-persistence`.
- Any change to chess rules.
- Windows support. Would require a console-API backend; out of scope.
- Truecolor, italics, underlines. The cell model reserves space for attributes; only what the board needs is implemented.

## Decisions

### Teardown is written before setup

The first code written is the restore path, tested by deliberately crashing. A program that dies without restoring leaves the user in a shell with no echo and no cursor — which reads to the user as "the terminal went crazy," the exact complaint this change exists to fix.

Restoration runs from `atexit` plus handlers for `SIGINT`, `SIGTERM`, and `SIGSEGV`. The handler uses only async-signal-safe calls: a pre-built restore string written with `write(2)`, and `tcsetattr`. No `printf`, no allocation.

`ISIG` stays enabled, so Ctrl-C still generates `SIGINT` and still quits. Alternative considered: disable `ISIG` and treat Ctrl-C as a key. Rejected — it makes the program unkillable by reflex if the event loop ever wedges.

### One reader, and the compiler enforces it

Every `wscanf`, `getwchar`, and `scanf` is deleted in this change. `input.c` owns the only `read(STDIN_FILENO, ...)` in the codebase. `display.c` from `modularize-and-build` is deleted rather than adapted, so no old prompting code survives to be called by accident.

Alternative considered: keep stdio for the menu and use raw reads only during play. That is precisely what `4f3ce77` did, and it is root cause #3.

### The parser's default branch discards

The parser is a small state machine over the byte stream:

```
byte ─┬─ not ESC ────────────────────────▶ Key
      └─ ESC ─┬─ '[' ─▶ CSI: consume params, then final byte 0x40–0x7E
              │          ├─ '<' … 'M'/'m' ─▶ Mouse (button 64/65 = wheel)
              │          ├─ 'A'/'B'/'C'/'D' ▶ Arrow
              │          ├─ "200~" … "201~" ▶ Paste
              │          └─ anything else ──▶ DISCARD
              ├─ 'O' ─▶ SS3: one more byte ─▶ Function key
              ├─ timeout (~25 ms) ─────────▶ bare Escape
              └─ else ─────────────────────▶ Alt+key
```

The important property is the last branch of CSI: unrecognized sequences are consumed *through their terminator* and produce nothing. A parser that gives up mid-sequence leaves the tail in the stream to be misread as keystrokes — that is root cause #2's mechanism.

SGR mouse mode (`?1006`) is used rather than the older X10 encoding, which cannot express coordinates beyond column 223.

### `?1007l` is not optional

Disabling alternate scroll is one byte sequence and it is root cause #1 in its entirety. Documented in the code with a comment naming what happens without it, because it looks removable and is not.

Alternative considered: leave it enabled and interpret the resulting arrow keys as scrolling. Rejected — it makes wheel and keyboard input indistinguishable, which is exactly the ambiguity that produced the original bug.

### Glyph width is measured, not assumed

At startup: clear a line, draw one piece glyph, query the cursor position with `\033[6n`, read the answer. The reported column minus one is the rendered width.

`wcwidth()` returns 1 for the Private Use Area codepoints the icons use, while many terminals render them at 2. Assuming either value breaks half of all terminals.

The query needs a bounded wait (~100 ms) — some terminals never answer — after which a documented default is used. The probe runs before the alternate screen is entered so its output does not appear in the game display.

### Diff-based flush

Two cell grids: the frame being composed and the frame on screen. Flush walks them together and emits only differing runs, coalescing adjacent changes and emitting a cursor-move only when the run is not contiguous with the last.

Alternative considered: clear and redraw each frame. That is the current code's `\033[2J\033[3J`, and it is why the display strobes. It also makes the alternate screen pointless.

A resize discards both grids and reallocates. The next frame is therefore a full repaint, which is correct and imperceptible.

### `render(rect)` is the resize strategy

No screen stores a width or height. The region is a parameter to every render call. This means resize handling is not implemented per screen — it does not exist per screen. Every screen written in changes ③, ④, and ⑤ is resize-correct on the day it is written.

This is the single highest-leverage decision in this change, and the reason the screen stack belongs here rather than in a later change.

### Screens return transitions

`handle` returns a `Cmd`; the app applies it between the event and the next frame. A screen never calls push or pop.

Alternative considered: let screens mutate the stack directly. Rejected — a screen that pops itself and then continues executing is reading freed memory, and that bug is invisible until it isn't. Returning a value makes it inexpressible.

Stack depth is fixed at 8, statically allocated, no per-push allocation. Chess never nests more than three deep.

### `sleep(1)` is replaced, not deleted

Its purpose was to let a player see their move before the board flips. Three mechanisms replace it, and together they serve the intent better than a fixed delay:

1. **Persistent last-move highlight.** The move stays visible after the flip, so seeing it is no longer a race against a timer. This is the actual fix.
2. **Explicit handoff.** `Black to move — press SPACE`. In pass-and-play the next player pressing a key *is* the handoff gesture, and it is paced by the human rather than by a constant.
3. **`f` to flip manually**, so auto-flip can be evaluated against not flipping at all.

### Minimum size

Board at measured cell width, plus a side panel, plus a status bar. Roughly 62×24 at single-width glyphs, wider at double. Computed rather than hardcoded, since it depends on the probe result.

## Risks / Trade-offs

- **A terminal disables alternate scroll differently, or ignores `?1007l`** → Test on Terminal.app, iTerm2, and at least one Linux terminal before considering this change done. If one ignores it, the fallback is to enable mouse tracking early so the wheel arrives as a mouse event we can consume.
- **`SIGSEGV` handler runs in a corrupted process and fails to restore** → Keep it to a single pre-built `write` and `tcsetattr`. Accept that a sufficiently broken crash may still leave a bad terminal; this reduces the window, it cannot close it.
- **The cursor-position probe hangs on a terminal that never answers** → Bounded wait plus a documented default. Never an unbounded read.
- **A resize arriving mid-escape-sequence corrupts the parse** → The parser's buffer survives `EINTR`; a short read resumes rather than restarting. Test by resizing while holding a key down.
- **The diff algorithm has a bug that leaves stale cells on screen** → Add a debug key that forces a full repaint, so it is possible to tell a rendering bug from a state bug during development.
- **This is the heaviest change and the one that killed the previous attempt** → Mitigated by the three root causes being identified rather than guessed at, and by ordering: teardown, then input, then rendering, then screens. Each is separately demonstrable.
- **Hand-rolled means owning terminal quirks forever** → Accepted knowingly. The escape hatch is ncursesw behind the same `term.h` interface, which is why the interface is narrow.

## Migration Plan

`display.c` is deleted, not adapted. `game.c` loses its printing and prompting and becomes the Game screen's `handle` and `render`. `main.c` shrinks to roughly 30 lines.

Rules stay untouched: the game remains exactly as incomplete as before, correctly displayed. This keeps the diff attributable — a bug appearing after this change is a rendering or input bug, never a rules bug.

Rollback is `git revert`; the change is self-contained and touches no persisted data.

## Open Questions

- Whether to auto-flip the board at all, or default to white's orientation with manual flip. Deferrable: `f` makes both reachable, and a preference can be added in `app-shell-and-persistence` once there is a settings screen to hold it.
- Whether the side panel sits right of the board or below it on narrow terminals. Deferrable: a layout detail that changes no requirement.
