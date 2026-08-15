## Purpose

Lets players take back a move — the ordinary courtesy of a game played across a table — and redo it if the take-back was itself a mistake.

## ADDED Requirements

### Requirement: Moves can be undone

The player SHALL be able to undo the most recent move, restoring the position in full: piece placement, side to move, castling rights, en passant availability, and the draw clocks.

Undo SHALL be repeatable back to the start of the game.

#### Scenario: Undo a move
- **WHEN** the player undoes the most recent move
- **THEN** the position returns to what it was before that move, and it is that side's turn again

#### Scenario: Undo a capture
- **WHEN** a capturing move is undone
- **THEN** the captured piece returns to the board and leaves the captured list

#### Scenario: Undo castling
- **WHEN** a castling move is undone
- **THEN** both king and rook return to their squares and the castling right is restored

#### Scenario: Undo en passant
- **WHEN** an en passant capture is undone
- **THEN** the captured pawn returns to its own square, not to the capturing pawn's destination

#### Scenario: Undo a promotion
- **WHEN** a promotion is undone
- **THEN** a pawn stands on the origin square, not the promoted piece

#### Scenario: Repeated undo
- **WHEN** the player undoes repeatedly
- **THEN** the game unwinds move by move to the starting position

#### Scenario: Nothing to undo
- **WHEN** no move has been played
- **THEN** undo is unavailable and is shown as such

### Requirement: Undone moves can be redone

The player SHALL be able to redo an undone move. Making a new move SHALL discard anything available to redo.

#### Scenario: Redo
- **WHEN** the player undoes a move and then redoes it
- **THEN** the position is identical to before the undo

#### Scenario: New move discards redo
- **WHEN** the player undoes a move and then plays a different one
- **THEN** the previously undone move can no longer be redone

#### Scenario: Nothing to redo
- **WHEN** no move has been undone
- **THEN** redo is unavailable and is shown as such

### Requirement: Undo is reflected everywhere

An undo SHALL update every view of the game consistently: the board, the captured pieces, the move history, the check indicator, and the last-move marking.

#### Scenario: Views agree
- **WHEN** a move is undone
- **THEN** the board, captured pieces, and history all reflect the earlier state
- **AND** the last-move marking shows the move before the undone one, or nothing if there is none

#### Scenario: Undoing out of check
- **WHEN** a move that answered a check is undone
- **THEN** the check indicator is shown again

#### Scenario: Undoing into a finished game
- **WHEN** a game has ended and the final move is undone
- **THEN** the game is in progress again and moves are accepted

### Requirement: Undo interacts correctly with saving

An undo SHALL be reflected in the autosaved game, so that resuming after an undo resumes the position the player left.

#### Scenario: Resume after undo
- **WHEN** the player undoes a move, quits, and relaunches
- **THEN** resuming gives the position as it stood after the undo
