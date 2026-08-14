## Purpose

Keeps a game safe across sessions without the player having to think about it, storing positions and moves in a portable text format that survives a rebuild, a different machine, and inspection by a human.

## ADDED Requirements

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

### Requirement: Games are saved automatically

The system SHALL save the game automatically after every completed move, without prompting.

The player SHALL NOT be interrupted to ask whether to save.

#### Scenario: Move completed
- **WHEN** a move is made
- **THEN** the game is saved without any prompt

#### Scenario: Unexpected termination
- **WHEN** the program is closed abruptly, or the terminal window is closed
- **THEN** every completed move up to that point is recoverable

#### Scenario: No periodic prompting
- **WHEN** a game runs for any number of moves
- **THEN** the player is never asked whether to save

### Requirement: Resuming

On startup, when a saved game exists, the system SHALL offer to resume it. Declining SHALL NOT delete it until a new game is actually started.

#### Scenario: Save present
- **WHEN** the program starts and an autosaved game exists
- **THEN** resuming it is offered

#### Scenario: Resuming
- **WHEN** the player resumes
- **THEN** play continues from the exact position, with the move history intact

#### Scenario: Declining then reconsidering
- **WHEN** the player declines to resume but does not start a new game
- **THEN** the saved game still exists and can be resumed later

#### Scenario: No save present
- **WHEN** no saved game exists
- **THEN** resuming is not offered

### Requirement: Named save slots

The system SHALL allow saving to a named slot and loading from one, independently of the autosave. Saving to a slot SHALL NOT end the game.

#### Scenario: Save and continue
- **WHEN** the player saves to a slot during a game
- **THEN** the game continues from the same position, with the same side to move

#### Scenario: Loading a slot
- **WHEN** the player loads a slot
- **THEN** play continues from the position it holds

#### Scenario: Overwriting
- **WHEN** the player saves to a slot that already holds a game
- **THEN** confirmation is required before the existing game is replaced

#### Scenario: Save failure
- **WHEN** a save cannot be written, for example because the location is not writable
- **THEN** the player is told, and the game continues uninterrupted
