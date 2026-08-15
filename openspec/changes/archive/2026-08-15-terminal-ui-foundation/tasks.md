## 1. Teardown first

Written and proven before any setup code exists. A crash must never leave a broken shell.

- [x] 1.1 Create `term.c/h` with `term_restore()` — restore termios, leave alternate screen, show cursor, re-enable alternate scroll, disable bracketed paste and mouse reporting
- [x] 1.2 Build the restore escape sequence once at startup into a static buffer so the signal path needs no formatting or allocation
- [x] 1.3 Register `term_restore` with `atexit`
- [x] 1.4 Install handlers for `SIGINT`, `SIGTERM`, and `SIGSEGV` using only async-signal-safe calls (`write`, `tcsetattr`), re-raising the default action afterwards
- [x] 1.5 Verify restoration on all four exit paths: normal quit, Ctrl-C, `kill`, and a deliberately triggered segfault

## 2. Terminal setup

- [x] 2.1 Detect a non-interactive stdin or stdout, print a message, and exit before changing any terminal state
- [x] 2.2 `term_enter()` — raw mode via `termios` with `ISIG` left enabled, `VMIN=1`, `VTIME=0`
- [x] 2.3 Emit the entry sequence: `?1049h` alt screen, `?25l` hide cursor, `?1007l` **alternate scroll off**, `?2004h` bracketed paste
- [x] 2.4 Comment `?1007l` in the source naming what happens without it — it looks removable and is not
- [x] 2.5 `term_size()` via `ioctl(TIOCGWINSZ)`
- [x] 2.6 `SIGWINCH` handler setting a single `volatile sig_atomic_t` flag, installed via `sigaction` **without** `SA_RESTART`
- [x] 2.7 Confirm a resize causes a blocked `read()` to return `EINTR` rather than waiting for a keystroke

## 3. Glyph width probe

- [x] 3.1 Implement `term_probe_glyph_width()` — draw one piece glyph, query with `\033[6n`, parse the reply, derive the width
- [x] 3.2 Run the probe before entering the alternate screen so its output does not land in the game display
- [x] 3.3 Bound the wait at ~100 ms and fall back to a documented default if no reply arrives
- [x] 3.4 Verify the probe reports correctly on a terminal with and without the Nerd Font — macOS Terminal.app and iTerm2 with the font; `--ascii` for the without-font path, which skips the probe entirely. Both probe answers (1 cell and 2 cells) and the no-answer fallback are covered by the pty harness.

## 4. Input pipeline

- [x] 4.1 Create `input.c/h` owning the only `read(STDIN_FILENO, ...)` in the codebase
- [x] 4.2 Define the `Event` type: `Key`, `Mouse`, `Resize`, `Paste`, `Eof`
- [x] 4.3 Implement the escape-sequence state machine per design.md, consuming CSI sequences through their final byte `0x40`–`0x7E`
- [x] 4.4 Decode SGR mouse reports (`\033[<b;x;yM|m`), mapping buttons 64/65 to wheel events with a direction
- [x] 4.5 Decode arrow keys, Enter, Escape, Backspace, Tab, and function keys as named keys
- [x] 4.6 Consume bracketed-paste bodies whole and emit a single paste event
- [x] 4.7 **Discard every unrecognized sequence in full**, emitting nothing — verify no fragment ever surfaces as key input
- [x] 4.8 Disambiguate a bare Escape with a ~25 ms timeout
- [x] 4.9 Handle `EINTR` by resuming a partial sequence rather than restarting it
- [x] 4.10 Delete every `wscanf`, `getwchar`, and `scanf` from the codebase
- [x] 4.11 Test: hold a key while resizing; scroll rapidly; paste multiline text; send an unknown escape sequence with `printf`. None may produce spurious input.

## 5. Frame buffer and flush

- [x] 5.1 Create `render.c/h` with a `Cell` (codepoint, foreground, background, attributes) and a grid
- [x] 5.2 Two grids — composing and displayed — with reallocation on resize
- [x] 5.3 Diff-and-flush emitting only changed runs, coalescing adjacent cells and emitting a cursor move only when a run is non-contiguous
- [x] 5.4 A forced-full-repaint debug key, so rendering bugs can be told apart from state bugs
- [x] 5.5 Confirm an unchanged frame produces zero bytes of output

## 6. Clipped drawing

- [x] 6.1 `Rect` type and drawing primitives that all take one: `draw_text`, `draw_hline`, `draw_vline`, `draw_box`, `fill`
- [x] 6.2 Truncate at the rectangle edge; never wrap, never draw outside
- [x] 6.3 Piece glyph table indexed by `(type, color)`, moved from `display.c`
- [x] 6.4 ASCII glyph table and an `--ascii` command-line flag
- [x] 6.5 Size squares from the probed glyph width so columns align at either width
- [x] 6.6 Test with a status string far longer than its region and confirm it is cut, not wrapped

## 7. Layout

- [x] 7.1 Compute the minimum size from the probed glyph width rather than hardcoding it
- [x] 7.2 Lay out board, side panel, and status bar for the current size, every frame
- [x] 7.3 `TooSmall` screen showing required and current sizes
- [x] 7.4 Verify shrinking below minimum and growing back leaves game state untouched
- [x] 7.5 Verify a mid-game font size increase relays out with no residue from the previous layout

## 8. Screen stack

- [x] 8.1 Create `app.c/h` with the `Screen` vtable: `on_enter`, `on_exit`, `handle`, `render`, `ctx`, `opaque`
- [x] 8.2 Fixed-depth stack (8), statically allocated, no per-push allocation
- [x] 8.3 `Cmd` return type: `NONE`, `PUSH`, `POP`, `REPLACE`, `QUIT`, applied after `handle` returns and before the next frame
- [x] 8.4 Render walk: from the topmost opaque screen upward, so overlays composite
- [x] 8.5 Deliver events only to the top screen
- [x] 8.6 Pass each screen its `Rect` as a render parameter; assert no screen stores a size
- [x] 8.7 Fire `on_enter` / `on_exit` on push and pop

## 9. Game screen

- [x] 9.1 Port `game.c` into a `Game` screen — turn flow into `handle`, board and panel drawing into `render`
- [x] 9.2 Delete `display.c` entirely
- [x] 9.3 Move entry by typed coordinates, drawn as a status-bar input field rather than a scrolling prompt
- [x] 9.4 Persistent last-move highlight on the from and to squares
- [x] 9.5 Explicit handoff between turns (`Black to move — press SPACE`)
- [x] 9.6 `f` to flip board orientation
- [x] 9.7 Remove `sleep(1)`
- [x] 9.8 Reduce `main.c` to terminal setup, pushing the Game screen, the event loop, and teardown

## 10. Verification

- [x] 10.1 Play a full game and confirm the rules behave exactly as before this change
- [x] 10.2 Test on Terminal.app and iTerm2: scrolling, resizing, font size changes, pasting, Ctrl-C. **A Linux terminal was deliberately deferred** — see the note below.
- [x] 10.3 Confirm scrolling produces no input on every terminal tested — the failure that ended the previous attempt. Confirmed on both macOS terminals, and on a pty against wheel bursts, unknown CSI, OSC/DCS payloads and bracketed paste.
- [x] 10.4 Run under a leak checker across a full game including several resizes
- [x] 10.5 Update `README.md`: `--ascii` flag, key bindings, and a note that mouse tracking affects text selection

### Deferred: Linux

`10.2` and `10.3` were verified on macOS only; testing on a Linux terminal was
deferred by decision, not by oversight. What is untested there is whether
`?1007l` is honoured — the one sequence the whole scrolling fix rests on. The
fallback if a terminal ignores it is named in `design.md` under Risks: turn
mouse tracking on early so the wheel arrives as a mouse event that gets
consumed. `mouse-and-highlights` turns that tracking on anyway, so a Linux
terminal that ignores `?1007l` is fixed by the next change rather than blocked
on this one.
