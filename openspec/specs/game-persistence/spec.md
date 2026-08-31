# game-persistence Specification

## Purpose

Keeps a game safe across sessions, storing positions and moves in a portable text format that survives a rebuild, a different machine, and inspection by a human.

**Status: revised from "automatic" to "explicit".** The original design saved after every move, silently, and offered to resume on launch. That was scrapped: an automatic background save resumed into a *finished* game (checkmate, a draw) exactly as if it were still in progress, with no result screen — the very outcome-hiding bug the app shell exists to prevent. Saving is now a conscious act (`s`, or the save option in the quit prompt), never automatic, so what gets loaded back is always the in-progress game the player actually asked to keep.

## Requirements

### Requirement: Portable text save format

A saved game SHALL be stored as text: the starting position followed by the moves played from it, and then whatever else is known about the game — its result if it has ended, the id that identifies it (see save-naming's A game is identified by more than its name), and its clock. The format SHALL NOT depend on the compiler, the machine's byte order, or the memory layout of any internal type.

A game that has ended SHALL record its result and the reason for it, because resignation, an agreed draw and running out of time leave no trace in the moves: replaying them produces a position that is merely unfinished, and a save of such a game would have nothing true to say about how it ended. A save with no recorded result SHALL be treated as a game still in progress.

A timed game SHALL record three things about its clock: the time control it is played under, how much time each side had left after each move that was played, and how much time each side has left as the file is written. The first is what makes the resumed game the same game; the second is what lets a replay show the clock as it stood at any point; the third is what lets a game saved in the middle of a turn resume with the mover's time already partly spent rather than restored to what it was at the start of the turn.

A save that records no time control SHALL be an untimed game, so that every file written before this format existed loads exactly as it did.

Everything after the moves SHALL be optional, so that a file holding only a position and a move list — hand-written, or produced by other software — still loads.

A saved game SHALL be readable and editable in a text editor, its clock included: times SHALL be legible as times rather than as an opaque encoding.

#### Scenario: Round trip
- **WHEN** a game is saved and loaded
- **THEN** the position, side to move, castling rights, en passant target, clocks, and full move list are all restored

#### Scenario: Finished game round trip
- **WHEN** a game that ended is saved and loaded
- **THEN** the result and its reason are restored along with the position and moves, including for a resignation, an agreed draw, or a game lost on time

#### Scenario: Timed game round trip
- **WHEN** a timed game is saved and loaded
- **THEN** its time control, both sides' remaining times, and the remaining times recorded for every move played are all restored

#### Scenario: Saved mid-turn
- **WHEN** a timed game is saved while the side to move has already spent part of their turn
- **THEN** loading it resumes with that time already spent, not with it returned

#### Scenario: A save with no time control
- **WHEN** a save file records no time control, including one written before the format carried one
- **THEN** it loads as an untimed game

#### Scenario: Portability
- **WHEN** a save file is moved to a different machine, or the program is rebuilt with different settings
- **THEN** the file still loads correctly

#### Scenario: Human readable
- **WHEN** a save file is opened in a text editor
- **THEN** the position, the moves, the clock, and anything else recorded after them are legible

#### Scenario: Externally authored position
- **WHEN** a file contains a valid position produced by other chess software
- **THEN** it loads and play may continue from it

#### Scenario: Position and moves only
- **WHEN** a save file records nothing after its moves
- **THEN** it loads as an untimed game in progress, with an id assigned the next time it is saved

### Requirement: Save files are validated

The system SHALL validate a save file before applying it and SHALL reject anything invalid with a message explaining the problem. A rejected file MUST NOT leave a partially loaded game.

Whatever is recorded after the moves SHALL be validated with the rest of the file. Text there that is not something the format defines is a malformed save, not something to skip over: a file half-understood is more dangerous than one refused.

A recorded clock SHALL be validated against the moves it accompanies and against the time control it is played under. A file whose per-move times do not number one per move played, or which records a per-move time or a remaining time that its time control could never have produced — a negative time, or one exceeding what the initial time plus every increment awarded so far could reach — SHALL be rejected. A file recording a clock but no time control, or a time control but no remaining time, SHALL be rejected as incomplete rather than half-applied.

#### Scenario: Truncated file
- **WHEN** a save file is incomplete
- **THEN** it is rejected with an explanation and the current game is unaffected

#### Scenario: Illegal move in the list
- **WHEN** a file contains a move that is not legal in the position it would be played from
- **THEN** the file is rejected rather than applied up to that point

#### Scenario: Not a save file
- **WHEN** the file is unrelated content
- **THEN** it is rejected with an explanation and no crash

#### Scenario: Older format
- **WHEN** a save file written by an earlier version is found
- **THEN** it is rejected with a message saying so, rather than misread

#### Scenario: Unrecognised trailing content
- **WHEN** a save file records something after its moves that the format does not define
- **THEN** it is rejected with an explanation, rather than loaded with that content ignored

#### Scenario: Unreadable result
- **WHEN** a save file records something in place of a result that is not a result
- **THEN** it is rejected with an explanation, rather than loaded as though the game were still in progress

#### Scenario: Wrong number of clock readings
- **WHEN** a save file records a number of per-move times that does not match the number of moves played
- **THEN** it is rejected with an explanation

#### Scenario: An impossible time
- **WHEN** a save file records a remaining time that is negative, or larger than its time control could ever have produced
- **THEN** it is rejected with an explanation

#### Scenario: A clock without a control
- **WHEN** a save file records remaining times but no time control, or a time control but no remaining times
- **THEN** it is rejected with an explanation, rather than loaded with the missing half guessed at

### Requirement: Saving is explicit

The system SHALL save a game only when the player asks it to — with the save command during play, by choosing to save when quitting, or by choosing to save a finished game from the result screen — never automatically and never on a fixed schedule. Saving an opening position with no moves played SHALL be refused, since there is nothing in it worth keeping.

This now covers finished games as well as games in progress. A finished game is worth keeping — it is the only kind that can be reviewed move by move — but keeping it is still the player's decision, made on the screen that tells them the game is over.

#### Scenario: Save command
- **WHEN** the player saves during a game
- **THEN** the game is written to disk and play continues, unaffected

#### Scenario: No moves to save
- **WHEN** the player tries to save before any move has been played
- **THEN** the save is refused with an explanation, and no file is written

#### Scenario: Quitting offers a save
- **WHEN** the player quits a game in progress
- **THEN** they are offered the choice to save before quitting, quit without saving, or cancel

#### Scenario: No unsolicited prompting
- **WHEN** a game runs for any number of moves without the player saving
- **THEN** the player is never prompted to save outside of quitting

#### Scenario: Saving a finished game
- **WHEN** the player chooses to save from the result screen
- **THEN** the finished game is written to disk and the result screen remains

#### Scenario: A finished game not saved
- **WHEN** the player leaves the result screen without choosing to save
- **THEN** nothing is written, and the game is gone

### Requirement: Named saves

The system SHALL allow saving the current game to a file at any time, and SHALL allow loading any such file, without ending the game in progress either way. The first save of a game SHALL create a new file named for that game; every later save of that same game — including one resumed by loading, and one saved again after it has ended — SHALL update that same file rather than creating another one.

Revised from an original three-slot design (three fixed slots proved not useful in practice), from an "always create a new file" revision after that (multiple saves of one game piled up as separate, indistinguishable files), and now from naming files by the clock. A timestamp answered "when did I press save", which is not a question anyone asks of a saved game; what the game is called and whether it can still be played are. See save-naming for how a name is chosen and kept unique.

**BREAKING**: a save's filename SHALL be its status — finished or ongoing — followed by its name. The clock time and the id are both gone from the name: the id lives in the file, and the time was precision nobody asked for. A file whose name does not follow this shape SHALL be listed under whatever its name is and loaded as a game in progress, so nothing already on disk becomes unreachable.

When a game that was saved in progress is saved again after it has ended, it SHALL take its finished name and the in-progress file SHALL NOT remain alongside it. The status changing is not a new game; it is the same game, in one file, described correctly.

The list of saved games SHALL show, for each, its status, its name, and how many moves it holds, in that order, and SHALL be ordered by when each was last saved, most recent first.

#### Scenario: First save
- **WHEN** the player saves a game for the first time
- **THEN** a new file is written, named by the game's status and the name it was given, and the game continues from the same position with the same side to move

#### Scenario: Saving again updates the same file
- **WHEN** the player saves a game that has already been saved once, in the same session or after loading it back
- **THEN** the existing file for that game is updated in place, and no additional file is created

#### Scenario: Distinct games stay distinct
- **WHEN** the player saves more than one different game
- **THEN** each has its own file, and saving one never touches another's

#### Scenario: Loading a save
- **WHEN** the player chooses a save from the list of them
- **THEN** play continues from the position it holds

#### Scenario: Listing saves
- **WHEN** the player opens the list of saves
- **THEN** every saved game is shown with its status, its name, and its number of moves, most recently saved first, scrollable if there are more than fit on screen

#### Scenario: Status in the name
- **WHEN** a saved game's filename is read without opening the file
- **THEN** its status and its name are both legible in it, and neither a clock time nor an id is there

#### Scenario: A game that ends after being saved
- **WHEN** a game saved in progress is saved again once it has ended
- **THEN** the list shows it once, as finished, and the in-progress file is gone

#### Scenario: A name in the old shape
- **WHEN** a save file's name follows the timestamped shape used before this change
- **THEN** it is listed under that name and loaded as a game in progress rather than skipped

#### Scenario: Save failure
- **WHEN** a save cannot be written, for example because the location is not writable
- **THEN** the player is told, and the game continues uninterrupted

### Requirement: A saved timed game is identifiable as one

The list of saved games SHALL show, for each timed game, the time control it was played under, so a game can be chosen without opening it. An untimed game SHALL be shown as it is today, with nothing in place of a time control.

#### Scenario: A timed game in the list
- **WHEN** the player opens the list of saves
- **THEN** each timed game shows its time control alongside its status, name and move count

#### Scenario: An untimed game in the list
- **WHEN** an untimed game appears in the list
- **THEN** it is shown exactly as before, with no time control and no placeholder for one
