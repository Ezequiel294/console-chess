## MODIFIED Requirements

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

## ADDED Requirements

### Requirement: A saved timed game is identifiable as one

The list of saved games SHALL show, for each timed game, the time control it was played under, so a game can be chosen without opening it. An untimed game SHALL be shown as it is today, with nothing in place of a time control.

#### Scenario: A timed game in the list
- **WHEN** the player opens the list of saves
- **THEN** each timed game shows its time control alongside its status, name and move count

#### Scenario: An untimed game in the list
- **WHEN** an untimed game appears in the list
- **THEN** it is shown exactly as before, with no time control and no placeholder for one
