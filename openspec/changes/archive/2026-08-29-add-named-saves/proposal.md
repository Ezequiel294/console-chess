## Why

A save file is currently named by the clock — `2026-08-16_223056-5d54b0.chess` — which tells the player when they pressed `s` and nothing about which game it was. With more than two or three saves the list becomes a wall of timestamps to open one by one. A game the player named is a game they can find. And a finished game cannot be saved at all today, so the games most worth keeping are the ones that are guaranteed to be lost.

## What Changes

- **Saving a game for the first time asks for a name.** A prompt takes the name; Enter on an empty field names the game after today's date; Escape cancels the save entirely and writes nothing. Later saves of the same game go to the same file without asking again.
- **The end screen can save a finished game.** The result screen's menu gains an option to save the game for review, using the same name prompt. Nothing is saved automatically — a finished game is kept because the player chose to keep it.
- **BREAKING: filenames become `<status>_<name>.chess`** — `ongoing_Sicilian.chess`, `finished_2026-08-17.chess`. The status leads so it is legible without opening the file; the name is what the player typed, or the date. Files in the old timestamped shape are listed under their old name as ongoing rather than disappearing.
- **Same-name saves are numbered.** When the filename a save wants is taken, `(1)` is appended, then `(2)`, and so on — the rule applies equally to unnamed games sharing a date and to a name the player typed twice. A game never collides with itself.
- **An unnamed game's date follows the calendar.** A game named after its date, resumed and saved on a later day, is renamed to that day; the file under the old date does not stay behind. A game the player named keeps its name.
- **Saving over the ongoing version.** When a game that was saved in progress is saved again as finished, it takes the finished name and the ongoing file is removed — one game, one file, whatever its status.
- **Finished saves record how the game ended**, since resignation and an agreed draw leave no trace in the moves; without it a resigned game is indistinguishable from an abandoned one.
- **Saves carry an id of their own**, inside the file rather than in its name. A name is for the player; the id is how the program knows the file it is about to overwrite is still the game it thinks it is, rather than a different match that has since taken that name.
- **The saved-games list shows status, name, and move count**, in that order, ordered most recently saved first. **BREAKING (minor)**: the side to move, currently shown per row, goes away — the requested row has three columns and this was not among them.
- **Games can be renamed from the saved-games list**, with the same prompt and the same numbering rules. Renaming a date-named game makes it a named one, so its date stops following the calendar.

## Capabilities

### New Capabilities
- `save-naming`: naming a saved game — the prompt, what an empty name means, what characters a name may contain, how a name becomes a filename, how collisions are numbered, when a name changes on its own, and renaming after the fact.

### Modified Capabilities
- `game-persistence`: the filename shape; the save format gaining an id and a recorded result; saving a finished game explicitly rather than never; the same-file rule extended across a status change; the saved-games list's columns and ordering.
- `app-shell`: the result screen's menu gains an option to save the finished game.

## Impact

- `src/app/save.c` / `save.h`: filename construction and parsing, collision numbering, the id and result lines, listing by modification time, rename, delete-after-rename.
- `src/app/` — a new text-input overlay for the name prompt; no such screen exists yet.
- `src/app/gameover.c`: a fourth menu option and the save it performs.
- `src/app/game.c`: `s` routes through the name prompt on a game's first save.
- `src/app/savedgames.c`: the three-column row, ordering, and the rename command.
- `src/types.h`: the game's id, whether its name was given or derived from a date, and its result carried on the game state.
- `tests/test_save.c`: the new filename grammar, numbering, the id and result lines, and old-format tolerance.

## Notes

This change lands before `add-game-replay`, which assumes finished games exist on disk and are distinguishable from unfinished ones. That change has been revised to depend on this one rather than to duplicate it: everything about how a game is saved and named lives here, and nothing in `add-game-replay` modifies `game-persistence` any more.
