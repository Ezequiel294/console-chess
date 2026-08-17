# game-persistence Specification

## Purpose

Keeps a game safe across sessions, storing positions and moves in a portable text format that survives a rebuild, a different machine, and inspection by a human.

**Status: revised from "automatic" to "explicit".** The original design saved after every move, silently, and offered to resume on launch. That was scrapped: an automatic background save resumed into a *finished* game (checkmate, a draw) exactly as if it were still in progress, with no result screen — the very outcome-hiding bug the app shell exists to prevent. Saving is now a conscious act (`s`, or the save option in the quit prompt), never automatic, so what gets loaded back is always the in-progress game the player actually asked to keep.

## Requirements

### Requirement: Portable text save format

A saved game SHALL be stored as text: the starting position followed by the moves played from it. The format SHALL NOT depend on the compiler, the machine's byte order, or the memory layout of any internal type.

A saved game SHALL be readable and editable in a text editor.

#### Scenario: Round trip
- **WHEN** a game is saved and loaded
- **THEN** the position, side to move, castling rights, en passant target, clocks, and full move list are all restored

#### Scenario: Portability
- **WHEN** a save file is moved to a different machine, or the program is rebuilt with different settings
- **THEN** the file still loads correctly

#### Scenario: Human readable
- **WHEN** a save file is opened in a text editor
- **THEN** the position and the moves are legible

#### Scenario: Externally authored position
- **WHEN** a file contains a valid position produced by other chess software
- **THEN** it loads and play may continue from it

### Requirement: Save files are validated

The system SHALL validate a save file before applying it and SHALL reject anything invalid with a message explaining the problem. A rejected file MUST NOT leave a partially loaded game.

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

### Requirement: Saving is explicit

The system SHALL save a game only when the player asks it to — with the save command during play, or by choosing to save when quitting — never automatically and never on a fixed schedule. Saving an opening position with no moves played SHALL be refused, since there is nothing in it worth keeping.

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

### Requirement: Named saves

The system SHALL allow saving the current game to a file at any time, and SHALL allow loading any such file, without ending the game in progress either way. The first save of a game SHALL create a new, distinctly named file; every later save of that same game — including one resumed by loading — SHALL update that same file rather than creating another one.

Revised from an original three-slot design (three fixed slots proved not useful in practice) and from an "always create a new file" revision after that (multiple saves of one game piled up as separate, indistinguishable files). Each game is tracked by an id assigned on its first save, embedded in the filename alongside the date it was first saved, so the file a game belongs to can be found again regardless of how many times it has been saved since.

#### Scenario: First save
- **WHEN** the player saves a game for the first time
- **THEN** a new file is written, named by the date and a freshly assigned id, and the game continues from the same position with the same side to move

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
- **THEN** every saved game is shown with when it was saved, scrollable if there are more than fit on screen

#### Scenario: Save failure
- **WHEN** a save cannot be written, for example because the location is not writable
- **THEN** the player is told, and the game continues uninterrupted
