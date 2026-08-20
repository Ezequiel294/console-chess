## 1. The save format

- [ ] 1.1 Add to `GameState` in `src/types.h`: the game's `id`, its `name`, whether that name was given or derived from a date, and its `Outcome_t result` (`OUTCOME_IN_PROGRESS` while it is being played). Keep `save_path` as the remembered file.
- [ ] 1.2 Teach `save_write` to emit the keyed trailer after the moves — `result`, `id`, `name` — per design.md, omitting `result` for a game in progress.
- [ ] 1.3 Teach `save_read` to parse the trailer: keys in any order, all optional; absent `result` means in progress; absent `id` means a file written by hand or before ids; absent `name` means auto. An unknown key, a malformed value, or a second copy of a key is `SAVE_READ_NOT_A_SAVE_FILE` with nothing partially loaded.
- [ ] 1.4 Add a narrow reader that returns just the id from a file, for the "is this file our own game?" check, without building a whole `GameState`.
- [ ] 1.5 Extend `tests/test_save.c`: a finished game round-trips its result and reason (including resignation and agreed draw, which the moves cannot reveal); a two-line file still loads as in progress and keeps working; an unknown trailer key is rejected; a malformed result is rejected.

## 2. Filenames

- [ ] 2.1 Replace `save_new_game_path` with a builder that takes status and name and produces `games/<status>_<name>.chess`.
- [ ] 2.2 Add the parser: split the stem at the first underscore into status and name; a first field that is neither `ongoing` nor `finished` means an ongoing game whose name is the whole stem, which is what makes old timestamped files still list.
- [ ] 2.3 Add collision numbering: if the target path exists and does not carry this game's id, try `(1)`, `(2)`, … and take the lowest free one.
- [ ] 2.4 Add name validation: reject `/`, `\` and control characters; trim surrounding whitespace; treat an all-whitespace name as empty; cap at 32 codepoints. Empty means the current date in `YYYY-MM-DD`.
- [ ] 2.5 Test the grammar directly: names containing underscores, spaces, parentheses and non-ASCII round-trip through build-then-parse; old-shape stems parse as ongoing; numbering picks the lowest free suffix and skips the game's own file.

## 3. One save entry point

- [ ] 3.1 Write the single save function described in design.md — determine name (recomputing the date for an auto name), build and number the target path, verify the remembered file still carries this game's id and forget it if not, write, then remove the superseded file only after a successful write, then update the remembered path.
- [ ] 3.2 Route `game.c`'s `save_game_now` through it for a game that already has a file.
- [ ] 3.3 Confirm the three flows it now unifies each work: a plain re-save, an auto-named game saved on a later day, and a game saved ongoing then saved again as finished — each leaving exactly one file.

## 4. The name prompt

- [ ] 4.1 Add `src/app/prompt.c` / `prompt.h`: a titled text-input overlay taking an initial value, a message line, and a callback with the submitted text; Enter submits, Escape cancels. Keep it to appending printable codepoints and removing one whole codepoint on backspace.
- [ ] 4.2 Enforce the length cap as the player types; validate characters on submit, showing which are not allowed and staying open.
- [ ] 4.3 Give it a hint line on the screen's last row, per app-shell's one-hint-line rule, replacing the hint of whatever is beneath it.
- [ ] 4.4 Check it renders and behaves at a small terminal size and across a resize, since it is drawn from the region handed in like every other screen.

## 5. Naming a game on its first save

- [ ] 5.1 Make `s` push the prompt when the game has no file yet, and save from the callback; a cancelled prompt writes nothing and leaves the game unsaved.
- [ ] 5.2 Do the same for the quit picker's "Save and quit", so quitting an unsaved game names it before exiting.
- [ ] 5.3 Confirm the refusal to save a game with no moves still happens before the prompt, not after — the player should not name a game only to be told there is nothing to save.

## 6. Saving a finished game

- [ ] 6.1 Set `state->result` from the outcome in `gameover_screen` before anything reads it.
- [ ] 6.2 Add "Save game" as the first of four options on the result screen, extending `GAMEOVER_ITEM_COUNT` and the click hit-testing that goes with it.
- [ ] 6.3 Save through the same entry point, with the finished status, showing the prompt when the game has never been saved and skipping it when it has; report the outcome in the screen's message area.
- [ ] 6.4 Verify the ongoing file is gone afterwards and the finished one carries the same id.
- [ ] 6.5 Verify that leaving the result screen without saving writes nothing at all.

## 7. The saved-games list

- [ ] 7.1 Replace `Saved_game_entry_t.label` with status, name and modification time; fill status and name from the filename, not from the file's contents.
- [ ] 7.2 Order by modification time, most recent first.
- [ ] 7.3 Draw the row as status, then name, then move count; the side to move goes away.
- [ ] 7.4 Update the screen's hint line and its empty-state text to match what saving now involves.

## 8. Renaming

- [ ] 8.1 Add a rename command to the saved-games list, opening the prompt with the current name as its initial value.
- [ ] 8.2 Perform the rename through the same save flow with the new name and a given-name flag, so numbering, the file rewrite, and the removal of the old file are the code already written.
- [ ] 8.3 Report a failed rename and leave the game listed under its original name.
- [ ] 8.4 Confirm a renamed date-named game keeps its new name when saved on a later day.
- [ ] 8.5 Add the rename key to the list's hint line and to help.

## 9. Finishing up

- [ ] 9.1 Run the full test suite; everything that was passing must still pass.
- [ ] 9.2 Drive the binary end to end: save an unnamed game, save a second one the same day, name a third, resume one and save it again, finish a game and save it from the result screen, rename something, and read the resulting `games/` directory. (Per the project memory: drive it through a Python pty and reconstruct the frame — tmux is not installed.)
- [ ] 9.3 Check the awkward cases by hand: cancelling the prompt on a first save, a name that is only spaces, a name with `/` in it, a 32-character name, and a name with accented characters.
- [ ] 9.4 Bump `VERSION` (currently 3.0.0) — the filename shape and the file format both change.
- [ ] 9.5 Delete the two files in `games/`, which the user has said are disposable, so the repo does not ship saves in a name shape that no longer exists.
