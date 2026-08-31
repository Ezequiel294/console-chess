## Purpose

Puts a real chess clock beside the board: a time control chosen before the game, one side's time running at a time, the handover between turns acting as the clock button, and a game that can end because someone ran out of time rather than because of anything on the board.

## ADDED Requirements

### Requirement: A game has a time control

A game SHALL be played under a time control chosen before it starts: an initial time each side begins with, and an increment added to a side's clock each time that side completes a turn. A game MAY instead be untimed, which is a game with no clock at all rather than a clock set to a very large number.

Both sides SHALL receive the same initial time and the same increment. The time control SHALL NOT change once the game has begun.

An increment of zero SHALL be permitted and SHALL mean no time is ever added.

#### Scenario: Timed game begins
- **WHEN** a game starts under a time control
- **THEN** both sides' clocks read the initial time, and neither has yet lost or gained any

#### Scenario: Untimed game
- **WHEN** a game starts with no time control
- **THEN** no clock runs, no clock is displayed, and no game can end on time

#### Scenario: The control is fixed
- **WHEN** a timed game is in progress
- **THEN** there is no way to change its initial time or its increment

### Requirement: Exactly one clock runs at a time

During live play of a timed game, the clock of the side whose turn it is SHALL run and the other side's SHALL be still. The two SHALL never run together and SHALL never both be stopped while the game is in progress and neither side is between turns.

A clock SHALL run against elapsed real time, and SHALL NOT depend on the player producing input: time passes while the player thinks, and the displayed time SHALL follow it without a keystroke.

Time SHALL be measured from a source that only ever moves forward, so that a change to the machine's wall clock — a time zone change, a clock correction — SHALL NOT add time to a player's clock or take it away.

#### Scenario: Time passes without input
- **WHEN** the player to move touches nothing for ten seconds
- **THEN** their clock has counted down by about ten seconds and their opponent's is unchanged

#### Scenario: The display keeps up
- **WHEN** a clock is running
- **THEN** the number on screen changes as the time does, with no keystroke needed to refresh it

#### Scenario: The other clock is still
- **WHEN** one side's clock is running
- **THEN** the other side's reading does not change at all

#### Scenario: The system clock changes
- **WHEN** the machine's wall clock jumps forward or backward during a game
- **THEN** neither player's remaining time is affected

### Requirement: The handover is the clock press

The turn handover SHALL act as the press of a chess clock. The player who has just moved SHALL keep spending their own time until the handover gesture is made; that gesture SHALL, in one step, add the mover's increment to the mover's clock, start the incoming player's clock, and flip the board.

The handover SHALL remain in an untimed game, where it does nothing to any clock and exists only so a player can see their move before the board turns.

A move that ends the game SHALL NOT require a handover: the game is over, so both clocks stop at the move rather than waiting for a press that would start nobody.

#### Scenario: Time until the press
- **WHEN** a player completes a move and does not immediately hand over
- **THEN** their own clock keeps counting down and the opponent's stays still

#### Scenario: The press
- **WHEN** the handover gesture is made
- **THEN** the mover's increment is added to the mover's clock, the incoming player's clock starts, and the board flips — all at that moment

#### Scenario: Handover in an untimed game
- **WHEN** a move completes in an untimed game
- **THEN** the handover still waits for the incoming player exactly as before, and no clock is involved

#### Scenario: A move that ends the game
- **WHEN** a move delivers checkmate, stalemate, or any other conclusion
- **THEN** both clocks stop at once and no handover is asked for

#### Scenario: The first turn
- **WHEN** a timed game opens on the board
- **THEN** the side to move is already running, with no press needed to begin

#### Scenario: Resuming a loaded game
- **WHEN** a saved timed game is loaded and appears on the board
- **THEN** the side to move resumes running from the time they had left, with no press needed

### Requirement: A clock runs until the turn is over, not until the screen is clear

A running clock SHALL continue to run while any overlay is open over the game — help, the move list, the promotion picker, a resignation or draw-offer confirmation, a save prompt, or the quit picker. Opening one of these is spending your own turn, exactly as leaving a real board is.

A clock SHALL be held only when the game is not playable at all: while the terminal is too small for the game to be displayed, no time SHALL be charged to either side.

#### Scenario: Thinking behind an overlay
- **WHEN** the player to move opens the move list, waits, and closes it
- **THEN** the time spent has come off their own clock

#### Scenario: Promotion is part of the move
- **WHEN** the player to move is choosing what a pawn promotes to
- **THEN** their clock is still running

#### Scenario: The terminal is too small
- **WHEN** the terminal is shrunk below the size the game needs and later enlarged again
- **THEN** neither side has been charged for the time it was unplayable

### Requirement: Running out of time ends the game

When a side's clock reaches zero, the game SHALL end immediately as a loss on time for that side, without waiting for either player to press anything. If the opponent holds no material with which checkmate could ever be delivered, the game SHALL instead end as a draw.

A clock SHALL NOT go below zero, and a side whose clock has reached zero SHALL NOT be able to complete a move.

The ending SHALL be reported the same way every other ending is: stated on the board screen and carried to the result screen, naming running out of time as the reason.

#### Scenario: Flag falls
- **WHEN** the running clock reaches zero
- **THEN** the game ends at once as a win for the opponent, with running out of time given as the reason

#### Scenario: Nobody is at the keyboard
- **WHEN** a clock reaches zero and no key is pressed
- **THEN** the game has still ended, and the screen says so rather than continuing to offer a move

#### Scenario: Opponent cannot mate
- **WHEN** a clock reaches zero and the opponent has only a lone king, or a king with a single bishop or knight
- **THEN** the game ends as a draw rather than a win on time

#### Scenario: No negative time
- **WHEN** a clock has reached zero
- **THEN** it reads zero and never a negative number

#### Scenario: Nothing after the flag
- **WHEN** a side has run out of time
- **THEN** no further move is accepted from either side

### Requirement: The clocks are drawn beside the board

A timed game SHALL display both clocks as two rectangles in the space to the right of the board and the existing captures-and-moves panel, one above the other with a gap between them, together spanning the board's height.

White's rectangle SHALL have a light background with dark digits and Black's a dark background with light digits, so that which clock belongs to which side is legible from the rectangle alone, whatever position it is in.

The lower rectangle SHALL always belong to the side the board currently faces, and the pair SHALL turn with the board — the near player's clock is nearest them, exactly as their pieces are.

Each rectangle SHALL also state which side it belongs to, so that the two are distinguishable on a terminal drawing no colour.

An untimed game SHALL show no clock rectangles, and its layout SHALL be exactly what it is without this capability.

#### Scenario: Both clocks visible
- **WHEN** a timed game is on the board
- **THEN** two rectangles are shown to the right of the board, spanning its height between them, each reading one side's remaining time

#### Scenario: Which is which
- **WHEN** the clocks are displayed
- **THEN** White's is light with dark digits, Black's is dark with light digits, and each names its side

#### Scenario: The near clock is the low one
- **WHEN** the board is facing one player
- **THEN** that player's clock is the lower of the two

#### Scenario: The clocks turn with the board
- **WHEN** the board flips at a handover
- **THEN** the two clocks exchange positions with it, and the incoming player's is now the lower

#### Scenario: No colour
- **WHEN** the terminal draws no colour
- **THEN** each clock is still identifiable as White's or Black's

#### Scenario: Untimed
- **WHEN** an untimed game is on the board
- **THEN** no clock rectangle is drawn and the captures-and-moves panel has the full width beside the board

#### Scenario: Not enough width
- **WHEN** the terminal is wide enough for the game but not for the clock rectangles beside the panel
- **THEN** both remaining times are still shown in a compact form, and no timed game is ever played with its clocks invisible

### Requirement: A clock reads as a time, at the precision that matters

A clock SHALL be displayed as minutes and seconds. Below ten seconds it SHALL be displayed to a tenth of a second, and above an hour it SHALL include the hours. It SHALL never be displayed as a bare number of seconds or milliseconds.

A displayed time SHALL never be rounded up: a clock showing `0:01` has at least one second left.

#### Scenario: Ordinary reading
- **WHEN** a side has nine minutes and forty-seven seconds left
- **THEN** the clock reads `9:47`

#### Scenario: The last ten seconds
- **WHEN** a side has less than ten seconds left
- **THEN** the clock reads to a tenth of a second, so the final seconds are distinguishable from one another

#### Scenario: A long game
- **WHEN** a side has more than an hour left
- **THEN** the clock includes the hours

#### Scenario: Never rounded up
- **WHEN** a side has any time at all left
- **THEN** the clock does not read zero
