# move-history-view Specification

## Purpose

Shows the moves played so far in readable chess notation, at any point during a game rather than only after it ends, with scrolling for games longer than the screen.

## Requirements

### Requirement: History is viewable during play

The move history SHALL be viewable at any time during a game, not only once it has ended. Viewing it SHALL NOT alter the position or the side to move.

#### Scenario: Opened mid-game
- **WHEN** the player opens the history during a game
- **THEN** every move played so far is listed

#### Scenario: Dismissing
- **WHEN** the player leaves the history
- **THEN** the game resumes exactly as it was, including any selection

#### Scenario: Empty history
- **WHEN** the history is opened before any move has been played
- **THEN** it indicates that no moves have been played, rather than showing an empty area

### Requirement: Moves are shown in standard notation

Moves SHALL be listed in standard algebraic notation, numbered, and paired by turn with White's move beside Black's.

#### Scenario: Numbered pairs
- **WHEN** several full moves have been played
- **THEN** each is numbered with both sides' moves on one line

#### Scenario: White to move
- **WHEN** White has moved and Black has not yet replied
- **THEN** the line shows White's move with Black's side of the pair empty

#### Scenario: Special moves
- **WHEN** the history includes castling, a capture, a promotion, or a checkmating move
- **THEN** each is shown in its standard notation

### Requirement: History scrolls

When the history is longer than the space available, the system SHALL allow scrolling through it by keyboard and by mouse wheel.

Wheel events received by the history SHALL scroll it.

#### Scenario: Long game
- **WHEN** the history exceeds the visible area
- **THEN** it can be scrolled to reach every move

#### Scenario: Wheel scrolling
- **WHEN** the player scrolls the wheel over the history
- **THEN** the list scrolls
- **AND** no keyboard input is produced as a side effect

#### Scenario: Keyboard scrolling
- **WHEN** the player uses the arrow keys, page keys, or home and end
- **THEN** the list scrolls accordingly

#### Scenario: Bounds
- **WHEN** the player scrolls past either end
- **THEN** the list stops at that end and does not scroll beyond it

#### Scenario: Opening position
- **WHEN** the history is opened
- **THEN** it is scrolled to show the most recent moves

#### Scenario: Resized while open
- **WHEN** the terminal is resized while the history is shown
- **THEN** the list is laid out for the new size and remains scrolled to a sensible position

### Requirement: History reflects undone moves

When moves are undone, the history SHALL no longer list them as played.

#### Scenario: After undo
- **WHEN** a move is undone and the history is opened
- **THEN** that move is not listed among the moves played
