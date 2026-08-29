## Context

See proposal.md — Why. The state of the code this has to fit into:

- **`save_new_game_path` builds the only filename shape there is** — `games/YYYY-MM-DD_HHMMSS-<id>.chess` — and `label_from_stem` parses it back with an `sscanf` that stops after the fields it names, falling back to the stem verbatim. `save_list_games` sorts by path string, which happened to be chronological because the timestamp led.
- **A game already remembers its file.** `GameState.save_path` is set on the first save and carried across a load (`savedgames.c` writes it into the loaded state), so "save to the same file" works today. What it cannot do is notice that the file at that path is no longer the game it thinks.
- **The save format is exactly two lines** — FEN, then coordinate moves — and `save_read` replays every move through the legal move generator. The old binary format is detected by a four-byte magic that no legal FEN can begin with.
- **There is no text input anywhere in the program.** Every existing screen is arrow keys and Enter; `game.c`'s typed-square field is the closest thing, and it accepts two characters of a fixed alphabet. The name prompt is the first real text field.
- **`gameover.c` has three menu options** in a fixed-size array (`GAMEOVER_ITEM_COUNT 3`) with a `row_y` per option for click hit-testing.
- **`input.c` decodes UTF-8 into codepoints** (`ev->key.ch` is a `uint32_t`), and `draw_text` renders one cell per codepoint.

## Goals / Non-Goals

**Goals:**

- A name is what the player typed, byte for byte, in the list and on disk.
- No save can destroy a different game's save, whatever names collide or change.
- One game, one file, across a status change and across a rename.
- A file that is only a position and a move list still loads.

**Non-Goals:**

- Deleting a saved game from the list. Adjacent, frequently wanted, and not asked for here.
- Folders, tags, or search over saves. The list is a list.
- Keeping the old timestamped filenames working as anything other than "listed and loadable as ongoing". They are not renamed or migrated.
- Reviewing a finished game. This change makes finished games exist on disk; `add-game-replay` is what opens them.

## Decisions

### The file gains a keyed trailer, after the moves

```
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
e2e4 e7e5 d1h5 b8c6 f1c4 g8f6 h5f7
result 1-0 checkmate
id 5d54b0
name given
```

Line 1 is the FEN and line 2 the moves, exactly as now. Everything after is zero or more `key value` lines, order-independent:

- `result <score> <reason>` — score is `1-0`, `0-1` or `1/2-1/2`; reason is one of `checkmate`, `stalemate`, `fifty-move`, `insufficient-material`, `repetition`, `resignation`, `agreement`, one per `Outcome_reason_t` except `OUTCOME_IN_PROGRESS`, which is spelled by the line's absence. The winner comes from the score, so resignation needs no extra field. The score is redundant with the reason in every case but resignation, and is kept because it is what makes the line legible to a human.
- `id <6 hex digits>` — the game's identity (see below).
- `name given` / `name auto` — whether the player typed the name or it stood in for one. Absent means `auto`.

An unknown key is a rejection, not something to skip: the existing format's whole stance is that a half-understood file is worse than a refused one.

*Why a trailer and not a header.* The position must stay on line 1 so a file that is only a FEN and moves — hand-written, or from other software — still loads, which is an existing requirement with an existing test. A header line would break it.

*Alternative — put the name in the file too.* Rejected: then the filename and the file could disagree about what a game is called, and there would be a question with no good answer about which wins. The name lives in the filename; the file records only whether that name was given or derived.

### Filenames: `<status>_<name>.chess`

`games/ongoing_Sicilian.chess`, `games/finished_2026-08-17(1).chess`. Parsed by splitting at the **first** underscore: everything before it is the status, everything after is the name. Names may therefore contain underscores freely. A stem whose first field is neither `ongoing` nor `finished` — which includes every file written before this change — is an ongoing game whose name is the whole stem.

Names allow any printable codepoint except `/`, `\`, and control characters, capped at 32 codepoints (buffered as 128 bytes, since a codepoint is up to four). Non-ASCII is allowed deliberately: `input.c` decodes UTF-8 and `draw_text` renders a codepoint per cell, so an accented name works end to end. The cap is enforced at the prompt, so an over-long name cannot be submitted and then truncated behind the player's back.

### Numbering is computed at write time, against the directory

Before writing, the target filename is built from status and name, and if a file already exists there **and is not this game's own file**, `(1)`, `(2)`, … are tried until one is free. "Its own file" is decided by id, not by path: read the candidate's `id` line and compare.

The number is part of the name from then on. It is *not* recomputed on every save — a game called `Sicilian(1)` stays `Sicilian(1)` even if `Sicilian` is later deleted — because a name that reshuffles when unrelated files come and go is not a name. It is recomputed only when the name itself is recomputed: a date-named game rolling onto a new day, or a game changing status into a collision.

*Alternative — a numeric suffix always, `Sicilian(0)`.* Rejected: the common case is one game per name, and `(0)` on every save is noise.

### One save entry point

Every save — `s` during play, the quit picker, the result screen — goes through one function that takes the state, the status it is being saved with, and whether a name has just been supplied:

1. Determine the name: an existing given name is kept; an auto name is recomputed from today's date.
2. Build the target path from status and name, numbering past collisions that are not this game's own file.
3. If the game has a remembered path, verify the file there still carries this game's id. If it does not — deleted, replaced, or a different game — forget it and treat this as a first save.
4. Write to the target path (`save_write` already writes to a `.tmp` and renames into place).
5. On success, if the old path differs from the target, remove the old file, then update the remembered path.

Removing after a successful write, never before, so a failure at step 4 leaves the previous save intact. This one ordering covers the date roll-over, the ongoing→finished transition, and the plain re-save, without any of them being a special case.

### The name prompt is a new, reusable overlay

A text-input screen (`src/app/prompt.c`): a titled box, a field, a message line for a rejected name, Enter to submit, Escape to cancel. It calls back with the submitted text, the same contract `savedgames_screen` uses for `on_loaded`, so the caller decides what happens next and the prompt knows nothing about saving.

It takes an initial value, which is what makes it serve the rename as well as the first save.

**It accepts letter keys, and that is not a violation of app-shell's "no letter shortcuts" rule.** That rule is about screens offering a *choice*, where a letter would be a shortcut to an option. This screen has no options; typing is what it is for. Worth stating because the rule reads as absolute and the next reader will check.

Validation happens on submit rather than per keystroke, except the length cap, which is enforced as you type — you cannot type a 33rd character, but you can type a `/` and be told it is not allowed. Rejecting `/` at the keystroke would leave the player pressing a key that does nothing with no explanation.

### `s` before the first save opens the prompt

`game.c`'s `save_game_now` splits: if the game has a remembered file, save straight to it as now; otherwise push the prompt and save from its callback. The quit picker's "Save and quit" does the same, which means quitting can now show a prompt before exiting — acceptable, since the alternative is quitting to a file named for nothing.

### The result screen gains a fourth option

`GAMEOVER_ITEM_COUNT` becomes 4, with "Save game" first — it is the one option that is lost by taking any of the others. The existing `row_y` array and click hit-testing extend unchanged. The option stays available after a successful save (saving twice is harmless and updates the same file); the outcome is reported in the screen's message area.

`gameover_screen` sets `state->result` from the outcome it was given before any of this, so the save records the ending.

### Listing by modification time

`save_list_games` calls `stat` per entry for `st_mtime` and sorts descending. It already opens and fully parses every file to count moves, so one more syscall per entry is not the cost worth avoiding. The row becomes `<status>  <name>  <n> moves`, with the side to move dropped — the requested row has three columns and that was not one of them.

`Saved_game_entry_t` gains `status`, `name`, and `mtime`, and loses `label`.

### Renaming rewrites the file

Rename reads the game, sets its name to given, writes it to the new path, and removes the old — the same five-step flow every save uses, with a name supplied. It has to touch the file rather than just rename it because the `name given` flag lives inside, and a rename must make a date-named game stop following the calendar.

## Risks / Trade-offs

- **The prompt is the first text input in the program, and text input is where terminal handling gets fiddly** — paste, Alt-modified keys, backspace over a multi-byte character. → Keep it deliberately small: printable codepoints append, backspace removes one whole codepoint, Enter and Escape end it, everything else is ignored. No cursor movement, no selection, no history.
- **Reading a candidate file's id to decide whether it is "our own" makes saving depend on parsing other files.** → The check is narrow: read the `id` line, compare, nothing else. A file that cannot be read is treated as not ours, which is the safe direction — it means numbering past it rather than overwriting it.
- **Quitting can now show a prompt**, which sits awkwardly with app-shell's "no unsolicited prompting" — though that rule is about prompts the player did not initiate, and this one follows directly from choosing "Save and quit". → Called out rather than resolved; if it grates in use, the alternative is naming the game before the quit picker appears.
- **A player who never names anything gets `2026-08-17`, `2026-08-17(1)`, `2026-08-17(2)`** — no better than timestamps for telling games apart. → Accepted: they had the chance to name it, and the date-plus-number is at least stable and short. The move count in the list is the tiebreaker.
- **Existing saves keep their old names and are listed as ongoing.** A player with old files sees rows whose "name" is a timestamp. → Accepted; they load and can be renamed.
