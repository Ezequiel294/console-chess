# board-interaction Specification

## Purpose

Lets a player choose a move by pointing at it — selecting a piece and then its destination — and unifies that with typed and keyboard input so every way of naming a square arrives at the same place.

## Requirements

### Requirement: Clicking identifies a square

A click within the board SHALL resolve to the square drawn at that position, under either board orientation and at either rendered glyph width.

A click outside the board SHALL resolve to no square. It MUST NOT resolve to the nearest square.

#### Scenario: Click on a square
- **WHEN** the user clicks within a drawn square
- **THEN** that square is the one identified

#### Scenario: Board flipped
- **WHEN** the board is shown from Black's orientation
- **AND** the user clicks the square drawn at the top-left of the board
- **THEN** the square identified is the one displayed there, not its mirror

#### Scenario: Click outside the board
- **WHEN** the user clicks on the side panel, the status bar, or empty space
- **THEN** no square is identified and no selection changes

#### Scenario: Double-width glyphs
- **WHEN** the terminal renders piece icons two cells wide, so squares are drawn wider
- **THEN** clicks still resolve to the square under the pointer

### Requirement: Selecting a piece

Clicking or otherwise naming a square holding a piece of the side to move SHALL select it. Selecting SHALL NOT move anything.

Naming a square that is empty or holds an opponent's piece, while nothing is selected, SHALL NOT select anything.

#### Scenario: Select own piece
- **WHEN** nothing is selected and the user names a square holding a piece of the side to move
- **THEN** that piece becomes selected

#### Scenario: Naming an opponent piece
- **WHEN** nothing is selected and the user names a square holding an opponent's piece
- **THEN** nothing is selected and the position is unchanged

#### Scenario: Naming an empty square
- **WHEN** nothing is selected and the user names an empty square
- **THEN** nothing is selected

### Requirement: Completing a move

With a piece selected, naming a square that is a legal destination for it SHALL make that move and clear the selection.

Naming a square that is not a legal destination SHALL leave the selection in place and indicate the move was rejected.

Naming another piece of the side to move SHALL move the selection to it rather than attempt a move.

#### Scenario: Legal destination
- **WHEN** a piece is selected and the user names one of its legal destinations
- **THEN** the move is made and nothing remains selected

#### Scenario: Illegal destination
- **WHEN** a piece is selected and the user names a square that is not a legal destination
- **THEN** no move is made
- **AND** the piece stays selected
- **AND** the rejection is indicated to the user

#### Scenario: Reselect
- **WHEN** a piece is selected and the user names a different piece of the side to move
- **THEN** the selection moves to the new piece and no move is made

#### Scenario: Cancel selection
- **WHEN** a piece is selected and the user presses Escape or names the selected square again
- **THEN** the selection is cleared and no move is made

#### Scenario: Move requiring promotion
- **WHEN** the selected pawn's destination is on the far rank
- **THEN** the user is asked which piece to promote to before the move is completed

### Requirement: Input methods are interchangeable

Selecting by mouse, by typing a square name, and by moving a keyboard cursor SHALL be equivalent. The resulting behaviour SHALL NOT depend on which was used, and all SHALL remain available at all times.

The game must stay fully playable without a mouse.

#### Scenario: Mixing methods within one move
- **WHEN** the user clicks a piece and then types its destination
- **THEN** the move is made exactly as if both had been clicked

#### Scenario: Keyboard only
- **WHEN** the user plays an entire game without using the mouse
- **THEN** every action, including castling and promotion, is reachable

#### Scenario: Typed coordinates still accepted
- **WHEN** the user types a square name as before this change
- **THEN** it behaves as it did

### Requirement: Castling by moving the king

Castling SHALL be offered as a destination of the king, requiring no separate command or notation.

#### Scenario: Castling offered
- **WHEN** the king is selected and castling is legal
- **THEN** the castling destination appears among its legal destinations

#### Scenario: Castling performed
- **WHEN** the user names the castling destination with the king selected
- **THEN** both king and rook move to their castled squares

### Requirement: Wheel events on the board

Wheel events received by the game screen SHALL be ignored. They MUST NOT alter the selection, make a move, or be treated as keyboard input.

#### Scenario: Scrolling during play
- **WHEN** the user scrolls the wheel while the board is shown
- **THEN** the game state and the selection are unchanged
- **AND** nothing appears on screen as a result
