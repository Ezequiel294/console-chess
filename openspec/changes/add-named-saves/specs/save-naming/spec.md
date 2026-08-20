## Purpose

Lets a saved game be called something the player will recognise later, and settles what happens when two games want the same name — so a list of saves reads as a list of games rather than a list of timestamps.

## ADDED Requirements

### Requirement: The first save asks for a name

The first time a game is saved, the system SHALL ask the player for a name for it. Saving that same game again SHALL NOT ask again; it goes to the file the game already has.

The prompt SHALL apply wherever a game is saved for the first time — the save command during play, saving when quitting, and saving a finished game from the result screen alike.

#### Scenario: Naming a game
- **WHEN** the player saves a game that has never been saved
- **THEN** they are asked for a name, and the game is saved under the name they give

#### Scenario: Saving again does not ask
- **WHEN** the player saves a game that already has a file, in the same session or after loading it back
- **THEN** no prompt appears and the existing file is updated

#### Scenario: The prompt does not disturb the game
- **WHEN** the name prompt is dismissed by any means
- **THEN** the position, the side to move, and any selection are exactly as they were before it opened

### Requirement: An empty name means today's date

Submitting an empty name SHALL name the game after the current date, in year-month-day order. A name consisting only of spaces SHALL be treated as empty.

Cancelling the prompt SHALL abandon the save entirely: no file is written, and the game is not considered saved. Cancelling is distinct from submitting nothing — one writes today's date, the other writes nothing at all.

#### Scenario: Empty name
- **WHEN** the player submits the name prompt with the field empty
- **THEN** the game is saved under the current date

#### Scenario: Only spaces
- **WHEN** the player submits a name that is nothing but spaces
- **THEN** it is treated as empty and the game is saved under the current date

#### Scenario: Cancelling
- **WHEN** the player cancels the name prompt
- **THEN** nothing is written, the game has no file, and the next save asks for a name again

#### Scenario: Surrounding spaces
- **WHEN** the player submits a name with spaces before or after it
- **THEN** those are discarded and the name is stored without them

### Requirement: Names are constrained to what a filename can hold

The system SHALL reject a name containing a character it cannot put in a filename, telling the player which characters are not allowed and asking again rather than altering what they typed. The name that reaches the disk SHALL be exactly the name the player submitted.

A maximum name length SHALL be enforced at the prompt, so a name cannot be entered that will not fit where it is displayed.

#### Scenario: Rejected characters
- **WHEN** the player submits a name containing a path separator or a control character
- **THEN** the prompt says so and waits for a different name, and nothing is written

#### Scenario: An accepted name is unaltered
- **WHEN** the player submits an accepted name
- **THEN** the saved game carries that name character for character, in the list and on disk

#### Scenario: Length limit
- **WHEN** the player has typed a name of the maximum length
- **THEN** further characters are not accepted, rather than being silently dropped at save time

### Requirement: A name that is taken is numbered

When the file a save would be written to already exists and belongs to a different game, the system SHALL append a number in parentheses to the name — `(1)`, then `(2)`, and so on, taking the lowest number not already in use — and save under that. This SHALL apply identically to a name the player typed and to a date standing in for one.

A game SHALL NOT collide with itself: re-saving a game never numbers it because its own file is already there.

#### Scenario: Second game of the day
- **WHEN** a second unnamed game is saved on a day that already has one
- **THEN** it is saved as that date followed by `(1)`

#### Scenario: Third game of the day
- **WHEN** a third unnamed game is saved on the same day
- **THEN** it is saved as that date followed by `(2)`

#### Scenario: A typed name that is taken
- **WHEN** the player names a game something another saved game of the same status is already called
- **THEN** the new one takes the next free number and the existing one is left untouched

#### Scenario: Re-saving does not number
- **WHEN** the player saves the same game repeatedly
- **THEN** its name is unchanged and no numbered copies appear

#### Scenario: A freed number is reused
- **WHEN** a numbered save is deleted and another game later wants that name
- **THEN** the freed number is available again, since the lowest unused number is taken

### Requirement: A date-named game follows the calendar

A game named after a date because the player gave no name SHALL be renamed to the current date each time it is saved, and the file under the previous date SHALL NOT remain. A game the player named SHALL keep that name however long it is played across.

Whether a name was given or stood in for one SHALL be recorded with the game, not guessed from whether the name looks like a date — a player who names a game "2026-08-17" has named it.

#### Scenario: Saved again the next day
- **WHEN** an unnamed game saved on one day is loaded and saved again on a later day
- **THEN** it is saved under the later date, and no file remains under the earlier one

#### Scenario: Saved again the same day
- **WHEN** an unnamed game is saved twice on the same day
- **THEN** its name is unchanged and there is one file

#### Scenario: A named game does not move
- **WHEN** a game the player named is saved on a later day
- **THEN** its name is unchanged

#### Scenario: A name that looks like a date
- **WHEN** the player types a name that happens to be a date
- **THEN** it is treated as a name they gave and does not change on a later day

#### Scenario: A date rolling into a taken name
- **WHEN** an unnamed game is saved on a later day on which another unnamed game of the same status already exists
- **THEN** it takes the next free number for that date

### Requirement: A saved game can be renamed

The system SHALL allow renaming a saved game from the list of saved games, with the same prompt, the same restrictions on characters, and the same numbering when the new name is taken. Renaming SHALL change what the game is called and nothing else about it — the moves, the position, and the result are untouched.

Renaming a date-named game SHALL make it a named game, so its name stops following the calendar.

#### Scenario: Renaming
- **WHEN** the player renames a saved game
- **THEN** it appears under the new name in the list, and loads exactly as it did before

#### Scenario: Renaming to a taken name
- **WHEN** the player renames a game to a name another saved game of the same status already uses
- **THEN** the renamed game takes the next free number

#### Scenario: Renaming a dated game
- **WHEN** the player renames a game that was named after a date
- **THEN** it keeps the new name on later saves rather than being renamed to the current date

#### Scenario: Cancelling a rename
- **WHEN** the player cancels the rename prompt
- **THEN** the game keeps its name and nothing on disk changes

#### Scenario: Rename failure
- **WHEN** a rename cannot be carried out, for example because the file is not writable
- **THEN** the player is told, and the game is still listed under its original name

### Requirement: A game is identified by more than its name

Each saved game SHALL carry an id of its own, recorded inside the file rather than in its name. Before overwriting a file it believes belongs to the game being saved, the system SHALL confirm that the file still carries that game's id; if it does not, the save SHALL be treated as a first save of a game with no file rather than overwriting a different game's save.

A name is for the player to recognise; it is not what tells two games apart. Two games can be called the same thing, a game can be renamed, and a date-named game changes its name on its own — none of which may be able to make one game's save destroy another's.

#### Scenario: The remembered file is a different game
- **WHEN** the file a game was saved to has been replaced by a different game's save
- **THEN** saving does not overwrite it; the game is saved as though for the first time

#### Scenario: The remembered file is gone
- **WHEN** the file a game was saved to no longer exists
- **THEN** saving writes it again rather than failing

#### Scenario: The id is not shown
- **WHEN** the player looks at the list of saved games
- **THEN** ids are nowhere in it; games are shown by status, name, and length

#### Scenario: A file with no id
- **WHEN** a save file carries no id, having been written before ids existed or by hand
- **THEN** it still loads, and is given an id the next time it is saved
