## 1. The clock, as arithmetic

Built and proven first, with no time source and no terminal, the way the rest of `core/` is. Everything after this is plumbing around a thing that already works.

- [ ] 1.1 Add `src/core/chessclock.{c,h}` with `Chess_clock_t` (initial_ms, increment_ms, remaining_ms[2], running side, started_at_ms) and `clock_init`, `clock_is_timed`, `clock_start`, `clock_press`, `clock_remaining`, `clock_expired_side` — every one taking `now_ms` as a parameter and reading no clock of its own. Verify it compiles into the test binary (the Makefile globs `src/core/*.c`, so no build change is needed).
- [ ] 1.2 Represent untimed as `initial_ms == 0` throughout, with `clock_is_timed()` the only test for it; verify no second flag exists that could disagree.
- [ ] 1.3 Implement `clock_remaining` as `base - (now - started_at)` for the running side and `base` for the other — computed, never accumulated — floored at zero so it never returns a negative. Verify with a test that calls it repeatedly with out-of-order and repeated timestamps and gets a consistent, never-negative reading.
- [ ] 1.4 Implement `clock_press`: charge the running side up to `now`, add its increment, store the result, start the other side at `now`. Verify a test asserting the mover's post-press remaining is `before - elapsed + increment` and the opponent's is untouched.
- [ ] 1.5 Add `clock_format(ms, buf, len)`: tenths below ten seconds, `m:ss` below an hour, `h:mm:ss` above, always truncating rather than rounding. Verify a table-driven test covering each boundary and asserting that any non-zero remainder never formats as `0:00`.
- [ ] 1.6 Add `tests/test_clock.c` declared in `tests/tests.h` and called from `tests/main.c`, covering 1.3–1.5 plus: an untimed clock never expires, a zero increment adds nothing, and a clock pressed with no time elapsed loses nothing.
- [ ] 1.7 Cover expiry in that suite: `clock_expired_side` reports the running side once `now` passes its remaining, reports `NONE` before that, and never reports the still side.

## 2. Telling the time, and losing on it

- [ ] 2.1 Add `uint64_t term_now_ms(void)` to `src/ui/term.{c,h}` over `clock_gettime(CLOCK_MONOTONIC)`. Verify by asserting it advances and never goes backwards across a short loop, and confirm it is the only place in the program that asks the OS for the time.
- [ ] 2.2 Add `outcome_can_mate(const Position *pos, Color side)` to `src/core/outcome.{c,h}`: false for a lone king, king+bishop, king+knight; true for everything else including king+two-knights. Verify a test covering each of those cases plus the asymmetric one (lone king vs. queen) where it disagrees with the both-sides insufficient-material draw.
- [ ] 2.3 Add `OUTCOME_TIMEOUT` and `OUTCOME_DRAW_TIMEOUT_INSUFFICIENT_MATERIAL` to `Outcome_reason_t`, constructed by the app layer only — `outcome()` sees no clock and must keep never returning either. Verify every existing `switch` over the enum compiles without a default case swallowing them.
- [ ] 2.4 Extend `outcome_message` (and whatever the result screen reads) to state losing on time and drawing on time distinctly. Verify by constructing each and reading the produced text.

## 3. Waking the loop up

- [ ] 3.1 Add `EV_TIMEOUT` to `Event_type_t` and `Event_t input_next_within(int timeout_ms)` in `src/ui/input.{c,h}`, with `input_next()` becoming `input_next_within(-1)`. Verify existing behaviour is unchanged by running the game and confirming keys, mouse, paste and resize all still work.
- [ ] 3.2 Implement the deadline as a budget over the whole decode loop, handing `term_read` `min(pending_timeout(), remaining budget)` each pass, so an expiry is never delayed past its deadline by an unrelated partial sequence.
- [ ] 3.3 Verify a partial escape sequence survives an expiry: bytes already in `g_buf` are left untouched and the sequence completes on a later call rather than being emitted as key input or dropped — the same guarantee `TERM_READ_INTR` already carries.
- [ ] 3.4 Add `int (*wake_in_ms)(void *ctx)` and `void (*tick)(void *ctx, uint64_t now_ms)` to `Screen`, both optional and NULL by default. Verify every existing screen still initialises cleanly and receives neither.
- [ ] 3.5 In `app_run`, take the minimum `wake_in_ms` over the whole stack (`-1` if none asks), wait that long, and call `tick` on every screen that has one before drawing. Verify with a temporary counter that a screen beneath an overlay is ticked and a screen without a `tick` is not.
- [ ] 3.6 Swallow `EV_TIMEOUT` in the loop — never dispatch it to `handle`. Verify no screen's `handle` ever observes it.
- [ ] 3.7 Confirm the idle case is untouched: in a menu, a replay, or an untimed game, `wake_in_ms` is `-1` everywhere and the loop blocks indefinitely, drawing no frames and consuming no CPU.

## 4. The clock on the game state

- [ ] 4.1 Add `Chess_clock_t clock` to `GameState` in `src/types.h` and `int32_t remaining_ms[2]` to `History_node_t`. Verify the test suite still builds and passes — `core/` must not need `chessclock.h` from `types.h`, so declare the struct where the include graph allows it without a cycle.
- [ ] 4.2 Have `update_history` take and store the two remaining times, and confirm every existing caller is updated. Verify `tests/test_make_unmake.c` and `test_replay.c` still pass.
- [ ] 4.3 Start the clock for the side to move when a live game screen is entered — a fresh game and a loaded one alike — with no press needed first. Verify by opening a timed game and watching White's clock start immediately, and by loading a saved timed game and watching the side to move resume.

## 5. Playing with a clock

- [ ] 5.1 Give the game screen a `wake_in_ms` returning 100 while a clock is running in live play and `-1` otherwise (untimed, replay, game over, terminal too small). Verify the loop idles in every one of those cases.
- [ ] 5.2 Implement the game screen's `tick`: consult `clock_expired_side` and, on expiry, set `game_over` / `pending_outcome` via the existing `apply_outcome` path — `OUTCOME_TIMEOUT` for the opponent, or the draw reason when `outcome_can_mate` is false for the opponent. Verify a short time control runs out with nobody touching the keyboard and the status line says so at that moment.
- [ ] 5.3 Make SPACE at the handover call `clock_press`, record the resulting pair onto the move's history node, then flip the board — one moment, in that order. Verify the increment lands on the mover and the opponent's clock starts only then.
- [ ] 5.4 Verify the mover keeps burning time between the move and the press, and that flagging during that gap ends the game as a loss for the mover.
- [ ] 5.5 Verify a game-ending move stops both clocks at the move and asks for no handover, and that an untimed game's handover is unchanged in every respect.
- [ ] 5.6 Verify the clock keeps running under every overlay — help, history, promotion, resign and draw confirmations, the save prompt, the quit picker — since none of them stops the turn.
- [ ] 5.7 Handle the too-small terminal: hold the clock while the game is undrawable and restart the running side's interval from `now` when it returns, charging nothing for the gap. Verify by shrinking the terminal for several seconds and checking both readings afterwards.
- [ ] 5.8 Refuse to complete a move for a side whose clock has reached zero, and accept no move from either side once the game has ended on time. Verify both.

## 6. Drawing the clocks

- [ ] 6.1 Add a `clock` rect to `Layout`, allocated from the width beyond `PANEL_MIN_W` beside the panel (`CLOCK_W` 12) and zero-width when there is not room. Verify `layout_min_cols` and `layout_min_rows` are numerically unchanged, so no terminal that plays today stops playing.
- [ ] 6.2 Draw the two rectangles spanning `layout.board.h` between them — `(h - 1) / 2` each with a one-row gap — each a box carrying its side's name and its formatted time. Verify they line up with the board's top and bottom edges at several terminal heights.
- [ ] 6.3 Colour White fg 232 on bg 254 and Black fg 252 on bg 236, fixed rather than drawn from `settings_palette()`. Verify they are unaffected by every colour scheme in Settings.
- [ ] 6.4 Drop the backgrounds when `term_supports_color()` is 0 and verify each clock is still identifiable as White's or Black's from its name alone (`NO_COLOR=1`).
- [ ] 6.5 Place the side matching `g->flipped` in the lower rectangle so the pair turns with the board. Verify across a handover in live play and across the manual flip in a replay.
- [ ] 6.6 Draw nothing in an untimed game and give the panel the full remaining width. Verify an untimed game's screen is pixel-identical to what it is before this change.
- [ ] 6.7 Implement the narrow fallback: when `clock` is zero-width, draw both times as two compact lines at the top of the panel. Verify by forcing `CLOCK_W` high enough to trigger it, then restoring.

## 7. Saving and loading the clock

- [ ] 7.1 Write the three trailer keys in `save_write` — `timecontrol <initial_ms> <increment_ms>`, `clocks <w,b> <w,b> …` one pair per move, `remaining <w,b>` at the moment of writing — and omit all three for an untimed game. Verify the produced file by eye in a text editor.
- [ ] 7.2 Parse them in `parse_trailer`, keeping its existing rejection of unknown and duplicate keys. Verify a file with no time control loads as an untimed game, including every save file already on disk.
- [ ] 7.3 Validate on load: `clocks` count equals the move count; no value negative or exceeding `initial + increment * moves_by_that_side`; `clocks`/`remaining` present only together with `timecontrol`. Reject with the existing `SAVE_READ_NOT_A_SAVE_FILE` rather than a new status. Verify a test per case in `tests/test_save.c`.
- [ ] 7.4 Verify a timed round trip in `tests/test_save.c`: time control, both remaining times, and every per-move pair come back identical.
- [ ] 7.5 Verify a mid-turn save round trip: a game saved with the mover partway through their turn resumes with that time already spent, not returned.
- [ ] 7.6 Show the time control for each timed game in the saved-games list, and nothing at all for an untimed one. Verify both appear correctly in a list holding some of each.

## 8. Replaying a timed game

- [ ] 8.1 Have the replay read each position's clocks from the history node it stepped to, displaying them without running. Verify stepping back and forth reproduces the same pair of readings every time.
- [ ] 8.2 Show the game's initial time on both clocks at the starting position. Verify by stepping all the way back.
- [ ] 8.3 Verify `wake_in_ms` is `-1` in a replay and that a replay left open for minutes shows unchanged readings and cannot end on time.
- [ ] 8.4 Verify a replay of a game lost on time ends with the loser's clock at zero and the result naming the loss on time.
- [ ] 8.5 Verify a replay of an untimed game draws no clocks and that stepping changes nothing in the clock area.
- [ ] 8.6 Verify the file a replay was read from is unchanged whatever position the replay is left on.

## 9. The setup screen

- [ ] 9.1 Add `src/app/newgame.{c,h}` with a row array (label, options, current index, enabled predicate) plus a `Start game` action row, and a completion callback in the shape `savedgames.c` already uses. Verify it opens and closes without starting a game.
- [ ] 9.2 Bind up/down to move between rows, left/right to change the highlighted row's value, and Enter to act only on the action row — so changing a setting can never start the game and moving between them can never change one. Verify each gesture in isolation.
- [ ] 9.3 Populate the time row (`Untimed, 1, 3, 5, 10, 15, 30, 60 min, Custom…`, default 10 min) and the increment row (`0, 1, 2, 3, 5, 10, 30 s, Custom…`, default 0). Verify confirming straight away starts a playable 10+0 game.
- [ ] 9.4 Wire `Custom…` to the existing `prompt_screen` with a numeric validator, rejecting anything that is not an amount of time with an explanation and leaving the settings untouched. Verify both a valid and an invalid entry.
- [ ] 9.5 Dim the increment row and ignore its left/right when the time row reads `Untimed` — shown as inapplicable, not removed, so the screen does not change shape. Verify by toggling between `Untimed` and a time.
- [ ] 9.6 Make a click highlight a row and nothing more, and put the drive instructions on the one bottom hint row. Verify against `app-shell`'s "a click only selects" rule.
- [ ] 9.7 Route `mainmenu.c`'s New Game and `gameover.c`'s new-game option through this screen instead of pushing `game_screen` directly, each returning to where it was opened from when backed out of. Verify both entry points and both back-out paths, and that backing out of the result screen's route ends nothing.
- [ ] 9.8 Verify adding one more row is all a future `Opponent` setting would take: no gesture, no start path, and no existing row changes. Record the check rather than adding the row.

## 10. Text, docs, and the whole thing together

- [ ] 10.1 Extend `src/app/help.c` with the clock: that SPACE is the clock press, that the mover's time runs until it, that overlays do not stop it, and that running out of time ends the game. Verify the help screen reads correctly in a timed game.
- [ ] 10.2 Update `README.md` — the setup screen, the clock, SPACE as the clock press, the new trailer keys in the save-format section, and the new termination reasons. Verify the key-bindings table and the Save and Load section both still describe what the program does.
- [ ] 10.3 Run `make test` and `make test-full`; verify everything passes, including the pre-existing perft and save suites.
- [ ] 10.4 Build with `make debug` (ASan/UBSan) and play a full timed game to a flag fall, a full timed game to checkmate, and a replay of each. Verify no sanitiser reports.
- [ ] 10.5 Play a full untimed game end to end and verify nothing about it differs from before this change — layout, handover, save file, and replay alike.
- [ ] 10.6 Save a timed game mid-turn, quit, reload, and verify both clocks resume where they were; then finish it, save it, and replay it, verifying the clocks track the moves.
- [ ] 10.7 Load a save file written before this change and verify it opens as an untimed game with no complaint.
