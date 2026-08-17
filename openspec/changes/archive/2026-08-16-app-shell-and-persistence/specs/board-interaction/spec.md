## Purpose

The board cursor — the keyboard's way of naming a square — becomes something the player summons and can see the effect of, rather than a permanent marker on the rank and file labels whose meaning nothing on screen explains.

## ADDED Requirements

### Requirement: The board cursor is present only while in use

The keyboard cursor SHALL NOT exist at the start of a turn. It SHALL come into being on the first arrow key press of that turn, at the centre of the board, and SHALL go away again once the square it named has been acted on or the selection is cancelled.

The first arrow press SHALL only summon the cursor; it SHALL NOT also step. Stepping from wherever the cursor was last left would make the same key do different things depending on history the player can no longer see.

The centre is the starting point because no square is more than four steps from it, and the player's own back ranks — where most of the pieces they are reaching for sit — are the nearer half.

Previously the cursor was always present, parked wherever it had last been left. Two rank and file labels were therefore always highlighted, with nothing on screen to say why, and no relationship to anything the player had done.

#### Scenario: Turn begins with no cursor
- **WHEN** a turn starts, including the first turn of a game and every turn after the board changes hands
- **THEN** no cursor is shown and no rank or file label is highlighted

#### Scenario: First arrow press summons it
- **WHEN** the player presses an arrow key and there is no cursor
- **THEN** the cursor appears at the centre of the board and does not move in the direction pressed

#### Scenario: Move completed
- **WHEN** a move is completed by any means
- **THEN** the cursor goes away, along with the highlighted labels

#### Scenario: Enter with no cursor
- **WHEN** the player presses Enter with nothing typed and no cursor
- **THEN** no square is named, since there is nothing on screen the press could be pointing at

### Requirement: The cursor marks the square, not just its labels

The cursor SHALL highlight the square it sits on as well as that square's rank and file labels, so what the highlighted labels refer to is visible on the board itself.

Naming a square by any means — a click, typed coordinates, or the cursor — SHALL put the cursor on it. A highlighted label therefore always means "this is the square in hand", whichever input method put it there, and the three methods stay indistinguishable in their result as Input methods are interchangeable requires.

#### Scenario: Clicking a piece highlights its labels
- **WHEN** the player clicks a piece
- **THEN** that square is highlighted and so are its rank and file labels

#### Scenario: Cancelling clears both
- **WHEN** the player clicks the selected piece again, releasing it
- **THEN** the square's highlight and its labels' highlights both go away

#### Scenario: The cursor is visible on the board
- **WHEN** the player moves the cursor with the arrow keys
- **THEN** the square under it is highlighted, not only its labels
