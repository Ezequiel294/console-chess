## Purpose

Shows the player what the rules already know — where a selected piece may go, which of those squares hold a capture, what just moved, and whether a king is under attack — so the state of the game is readable at a glance instead of held in the player's head.

## ADDED Requirements

### Requirement: Legal destinations are shown

While a piece is selected, every square it may legally move to SHALL be marked. Squares it may not move to SHALL NOT be marked.

The marked set SHALL be exactly the legal moves for that piece, so a square that looks available always is.

#### Scenario: Piece selected
- **WHEN** a piece is selected
- **THEN** each of its legal destinations is marked and no other square is

#### Scenario: Pinned piece
- **WHEN** a pinned piece is selected
- **THEN** only the destinations that keep its king shielded are marked

#### Scenario: While in check
- **WHEN** the side to move is in check and a piece is selected
- **THEN** only destinations that resolve the check are marked

#### Scenario: Piece with no moves
- **WHEN** a piece with no legal moves is selected
- **THEN** no destination is marked, and this is distinguishable from nothing being selected

#### Scenario: Selection cleared
- **WHEN** the selection is cleared or a move is made
- **THEN** no destination markings remain

### Requirement: Captures are distinguishable from quiet moves

A legal destination holding an opposing piece SHALL be marked differently from an empty one, so a capture is recognizable without inspecting the square.

#### Scenario: Capture available
- **WHEN** a selected piece may capture on a square
- **THEN** that square is marked in a way that differs visibly from an empty destination

#### Scenario: En passant destination
- **WHEN** an en passant capture is legal
- **THEN** the destination is marked as a capture, even though it is empty

### Requirement: The selected square is identifiable

The square holding the selected piece SHALL be marked distinctly from both unselected squares and destination markings.

#### Scenario: Selection visible
- **WHEN** a piece is selected
- **THEN** its own square is visibly distinct from every other square

### Requirement: The last move remains visible

The origin and destination of the most recent move SHALL stay marked until the next move is made, including after the board is flipped or the display is redrawn.

This is what allows a player to see what their opponent did without relying on a timed pause.

#### Scenario: After a move
- **WHEN** a move is made
- **THEN** both its origin and destination are marked

#### Scenario: Surviving a flip
- **WHEN** the board orientation changes after a move
- **THEN** the last move is still marked

#### Scenario: Replaced by the next move
- **WHEN** the following move is made
- **THEN** only the newer move is marked

#### Scenario: After a resize
- **WHEN** the terminal is resized
- **THEN** the last move is still marked

### Requirement: Check is indicated

When a side is in check, its king's square SHALL be marked, distinctly from selection, destination, and last-move markings.

#### Scenario: In check
- **WHEN** a side is in check
- **THEN** its king's square is marked

#### Scenario: Check resolved
- **WHEN** the check is answered
- **THEN** the marking is removed

#### Scenario: Checkmate
- **WHEN** the game ends in checkmate
- **THEN** the mated king's square remains marked in the final position

### Requirement: Markings work without colour

Every marking SHALL remain distinguishable on a terminal without colour support, using shape or character rather than colour alone.

#### Scenario: Monochrome terminal
- **WHEN** the game runs where colour is unavailable
- **THEN** selection, legal destinations, captures, last move, and check remain distinguishable from each other and from unmarked squares

#### Scenario: ASCII mode
- **WHEN** the game runs in ASCII fallback mode
- **THEN** all markings are still shown and still distinguishable

### Requirement: Markings never disturb the board

Markings SHALL NOT change the size, alignment, or position of any square, and SHALL NOT obscure the piece standing on a marked square.

#### Scenario: Marking an occupied square
- **WHEN** a square holding a piece is marked
- **THEN** the piece remains identifiable

#### Scenario: Alignment preserved
- **WHEN** any combination of markings is shown
- **THEN** every board column and row stays aligned
