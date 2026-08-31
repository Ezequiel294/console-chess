## MODIFIED Requirements

### Requirement: Every view follows the step

Stepping SHALL update everything the screen shows about the game together: the board, the captured pieces, the check indicator, the last-move marking, the clocks, and the side to move (or, at the final position, the result in its place — see The end of the game is stated, not screened).

#### Scenario: Views agree
- **WHEN** the player steps backward over a capture
- **THEN** the captured piece is back on the board and out of the captured list
- **AND** the last-move marking shows the move before it, or nothing at the starting position

#### Scenario: Check reappears
- **WHEN** the player steps back to a position in which a king was in check
- **THEN** the check indicator is shown again, as it was during the game

#### Scenario: Side to move
- **WHEN** the player steps to any position other than the final one
- **THEN** the side that was to move in that position is stated, whichever way the board is facing

#### Scenario: Clocks follow the step
- **WHEN** the player steps to any position in a replay of a timed game
- **THEN** both clocks read what each side had left at that point in the game

## ADDED Requirements

### Requirement: A replay's clocks are read, not run

In a replay of a timed game, both clocks SHALL be displayed and neither SHALL run. The times shown SHALL be those recorded for the position the replay is on — what the players actually had — rather than recomputed from the time control.

A replay SHALL NOT be able to end on time, whatever it is left showing, and leaving a replay on any position SHALL NOT alter the times recorded in the file it was read from.

At the starting position both clocks SHALL read the game's initial time, since that is what both sides had before anything was played.

A replay of an untimed game SHALL show no clocks, exactly as an untimed live game does.

#### Scenario: Clocks are still
- **WHEN** a replay of a timed game is left on any position for any length of time
- **THEN** neither clock's reading changes

#### Scenario: What the players saw
- **WHEN** the replay is on the position after a given move
- **THEN** the clocks read what each side had left after that move was played, including the increment the mover received

#### Scenario: At the start
- **WHEN** the replay is at the starting position
- **THEN** both clocks read the game's initial time

#### Scenario: A game that ended on time
- **WHEN** the replay of a game lost on time reaches its final position
- **THEN** the losing side's clock reads zero and the result states that the game was lost on time

#### Scenario: Untimed replay
- **WHEN** a replay of an untimed game is on screen
- **THEN** no clock is drawn, and stepping changes nothing about the clock area
