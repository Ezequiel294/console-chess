## Why

The game prints a stream of lines and clears the screen between them. It has no idea where anything is on screen, how large the terminal is, or when that size changes. Every UI complaint follows from that single fact: text wraps when a line exceeds the width, increasing the font mid-game destroys the display, and the previous attempt at mouse support had to clear the screen every iteration and hope the board landed on row 1.

That attempt also produced the failure that ended the project: scrolling injected junk into the game. The cause is now identified — three stacked problems, all fixable:

1. **Alternate scroll mode.** In the alternate screen buffer, terminals translate the scroll wheel into arrow-key sequences (`\033[A`) by default. Enabling the alt screen at `3d4a18e` did not cause this; it exposed it. The commit was reverted for a side effect it merely uncovered.
2. **Unhandled wheel reports.** With mouse tracking on, the wheel arrives as button **64**/**65**, which the old parser did not recognize, so the escape sequences fell through into the input stream as text.
3. **Two readers on one file descriptor.** `4f3ce77` kept `wscanf`/`getwchar` (stdio, line-buffered) alongside `read(STDIN_FILENO, ...)` in raw mode. Two buffers racing over the same bytes, which matches the reported "worked but not always."

This change builds the terminal layer properly, once, so the remaining three changes have solid ground.

## What Changes

**Terminal runtime (`term.c/h`)**

- Raw mode via `termios`, with `ISIG` left enabled so `Ctrl-C` still works.
- Enter sequence: `\033[?1049h` (alt screen), `\033[?25l` (hide cursor), `\033[?1007l` (**alternate scroll off** — fixes cause #1), `\033[?2004h` (bracketed paste, so a paste cannot arrive as keystrokes).
- **Guaranteed teardown**: `atexit` plus handlers for `SIGINT`, `SIGTERM`, and `SIGSEGV` that restore termios, leave the alt screen, and show the cursor. Written and tested before the setup path — a crash must never leave the user's shell broken.
- Size via `ioctl(TIOCGWINSZ)`; `SIGWINCH` handler sets a `volatile sig_atomic_t` flag and nothing else. Registered **without** `SA_RESTART` so a blocked `read()` returns `EINTR` and the loop handles the resize immediately rather than waiting for the next keypress.
- Startup probe for glyph width: draw a piece icon, query the cursor column with `\033[6n`, and record whether this terminal renders the Nerd Font Private Use Area codepoints as 1 or 2 cells. `wcwidth()` reports 1 for these codepoints while many terminals render 2, which shears every board row sideways.

**Input (`input.c/h`)**

- Exactly one reader on stdin. Every `wscanf`, `getwchar`, and `scanf` is deleted from the input path (fixes cause #3).
- **End of input terminates the program.** The current `get_move` spins forever when stdin reaches EOF: `wscanf` returns `EOF` without writing to its buffer, the input is judged invalid, and the `while (getwchar() != '\n')` drain loop then never sees a newline because `getwchar` returns `WEOF` on every call. Found while testing `modularize-and-build`, and left alone there because this change deletes the code that contains it — recorded here so it is not reintroduced. The read loop must treat EOF as a `Quit` event, which is also what makes the game scriptable for testing.
- An escape-sequence parser producing typed `Event` values (`Key`, `Mouse`, `Resize`, `Paste`, `Quit`). CSI sequences are consumed through their final byte `0x40`–`0x7E`; wheel buttons 64/65 are recognized and routed rather than leaked (fixes cause #2); bracketed-paste bodies are consumed whole; a bare `ESC` is disambiguated by a ~25 ms timeout.
- **Unknown sequences are consumed and discarded.** Game logic never sees a raw byte. This is the rule that makes injected junk structurally impossible rather than merely unlikely.

**Rendering (`render.c/h`)**

- A cell back buffer (codepoint, fg, bg, attrs). Frames are composed into it, diffed against the previous frame, and flushed as the minimal ANSI needed. No more `\033[2J` strobing.
- Widgets draw into a `Rect` handed to them and **truncate** rather than wrap. Overflowing text becomes structurally impossible.
- Layout computed from the current size each frame. Below the minimum, a "terminal too small — need N×M, have N×M" panel replaces the game.
- Piece glyph table plus an `--ascii` fallback mode (`KQRBNP` / `kqrbnp`) selectable at runtime, so the game works on terminals without a Nerd Font.

**Screen stack (`app.c/h`)**

- A `Screen` vtable (`on_enter`, `on_exit`, `handle`, `render`, `ctx`, `opaque`) and a fixed-depth stack, so overlays composite over the screen beneath and pop back to exactly where they were.
- Screens never mutate the stack directly. `handle` **returns** a `Cmd` (`NONE` / `PUSH` / `POP` / `REPLACE` / `QUIT`) that the app applies between frames, which makes a screen popping itself and then continuing to execute inexpressible.
- `render` receives its `Rect` as a parameter; no screen stores a width. Every screen written in later changes is resize-correct on the day it is born.
- Ships two screens: `Game` and `TooSmall`. Menus, history, and settings arrive in `app-shell-and-persistence`.

**Turn pacing**

- Remove `sleep(1)` (`main.c:181`) and replace it with what it was standing in for. The intent — let a player see their move before the board flips — is served better by a **persistent last-move highlight** (the move stays visible after the flip, so seeing it is no longer a race) plus an **explicit handoff** (`Black to move — press SPACE`), plus `f` to flip orientation manually.

## Capabilities

### New Capabilities

- `terminal-runtime`: terminal lifecycle — raw mode, alternate screen, mouse/paste/scroll mode setup, guaranteed teardown on exit and on signals, size queries, resize notification, glyph-width probing.
- `input-events`: single-reader input pipeline — byte stream to typed events, escape-sequence parsing, unknown-sequence discard, bracketed paste, `EINTR` handling.
- `screen-rendering`: frame composition — cell back buffer, diff-based flush, clipped widget rects with truncation, size-driven layout, too-small state, piece glyph table and ASCII fallback.
- `screen-navigation`: the screen stack — vtable contract, opaque vs. overlay compositing, returned-transition model, per-frame rect delivery.

### Modified Capabilities

None. No existing specs.

## Impact

- **Code**: `display.c` from `modularize-and-build` is replaced entirely. `game.c` loses its printing and prompting; it becomes the `Game` screen's `handle`/`render`. `main.c` shrinks to roughly 30 lines: set up the terminal, push the first screen, run, tear down.
- **Behavior**: chess rules are untouched — the game is exactly as (in)complete as before, rendered properly. Move entry still happens by typing coordinates; mouse arrives in `mouse-and-highlights`.
- **Dependencies**: none. Hand-rolled ANSI, no ncurses. Revisit only if this layer becomes unmaintainable.
- **Portability**: POSIX `termios`/`ioctl`/`signal`. macOS and Linux. Not Windows without a compatibility layer.
- **Risk**: highest of the five, and the one that ended the previous attempt. Mitigated by building teardown first and by the three named root causes above being understood rather than guessed at.
- **Downstream**: `mouse-and-highlights` needs the input pipeline and render geometry. `app-shell-and-persistence` needs the screen stack. `chess-rules-engine` needs the overlay mechanism for the promotion picker.
