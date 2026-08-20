## Purpose

Lets a finished game be walked back and forth on the board it was played on, so a player can see how the game actually unfolded position by position rather than reading a list of moves and imagining it.

## ADDED Requirements

### Requirement: A finished game opens as a replay

Choosing a finished saved game SHALL open it on the board in replay mode, positioned at the end of the game — the final position, exactly as the players left it. A saved game that is still in progress SHALL continue to open in live play, unchanged.

Replay mode SHALL be visibly distinct from live play: what a player can do differs, so what the screen offers must differ with it.

#### Scenario: Opening a finished game
- **WHEN** the player chooses a finished game from the list of saved games
- **THEN** it opens on the board showing the final position, in replay mode

#### Scenario: Opening an unfinished game
- **WHEN** the player chooses a saved game that is still in progress
- **THEN** play continues from the position it holds, as a live game, with the live game's commands

#### Scenario: The mode is apparent
- **WHEN** a replay is on screen
- **THEN** it is identifiable as a replay rather than a game in progress, without the player having to press a key to find out

### Requirement: Stepping through the played moves

The player SHALL be able to step backward and forward through the moves of the game one at a time. Stepping backward SHALL restore the position exactly as it stood before that move — piece placement, side to move, castling rights, en passant availability, and the draw clocks — and stepping forward SHALL restore it exactly as it stood after.

Both directions SHALL be reachable by two gestures: the undo and redo command keys, and the left and right arrow keys. The two SHALL be the same operation, not two behaviours that can disagree.

The reachable positions SHALL be exactly those the game actually passed through: the starting position, every position after a played move, and nothing else.

#### Scenario: Stepping back
- **WHEN** the player steps backward
- **THEN** the board shows the position before the most recent move on it, and it is that side's turn again

#### Scenario: Stepping forward
- **WHEN** the player steps forward after stepping back
- **THEN** the board shows the position it held before the step back, identically

#### Scenario: Arrows and command keys agree
- **WHEN** the player presses the left arrow, and separately presses the undo key
- **THEN** both step backward by one move, with the same result

#### Scenario: Stepping back to the start
- **WHEN** the player steps backward repeatedly
- **THEN** the game unwinds move by move to the starting position

#### Scenario: Before the first move
- **WHEN** the board is at the starting position
- **THEN** stepping backward does nothing, and is shown as unavailable

#### Scenario: After the last move
- **WHEN** the board is at the final position
- **THEN** stepping forward does nothing, and is shown as unavailable

#### Scenario: Special moves step cleanly
- **WHEN** the move stepped over is a castling move, a capture, an en passant capture, or a promotion
- **THEN** stepping back and forward across it leaves the position identical to what it was before the pair of steps

### Requirement: A replay cannot be played

No move SHALL be made in a replay. Naming a square, clicking a piece, or any other gesture that would originate a move SHALL NOT change the position; the only way the position changes is by stepping.

Consequently a replay SHALL NOT offer to save, to resign, or to offer a draw: there is nothing left to save, no game to resign, and no opponent to agree with.

#### Scenario: A piece cannot be picked up
- **WHEN** the player clicks a piece, or types a square, during a replay
- **THEN** no piece is selected and the position is unchanged

#### Scenario: A different move cannot be played
- **WHEN** the player has stepped back and attempts a move other than the one actually played
- **THEN** it is not accepted, and the only way forward remains the move that was played

#### Scenario: Live-play commands are absent
- **WHEN** the player presses the save, resign, or draw-offer key during a replay
- **THEN** nothing happens, and none of the three is listed among the replay's commands

#### Scenario: The saved game is untouched
- **WHEN** the player steps through a replay and leaves it
- **THEN** the file it was read from is unchanged, whatever position the replay was left on

### Requirement: The board's orientation is the reviewer's choice

A replay SHALL show the board in one orientation until the player flips it, and SHALL provide a flip command to do so. The orientation SHALL NOT change on its own — not when stepping to a move by the other side, and not on entering or leaving any position.

A replay SHALL NOT wait for a handover between moves. The turn handover exists so two players can pass one terminal between them; a replay has one person at it.

#### Scenario: Orientation holds while stepping
- **WHEN** the player steps forward or backward across any number of moves
- **THEN** the board stays in the orientation it was already in

#### Scenario: Flipping
- **WHEN** the player presses the flip command
- **THEN** the board turns, and stays turned for subsequent steps until flipped again

#### Scenario: No handover
- **WHEN** the player steps to another move
- **THEN** the new position is shown immediately, with nothing to acknowledge first

### Requirement: A replay's commands are its own

A replay SHALL offer: flip the board, step backward, step forward, view the move history, help, and quit. These SHALL be displayed the way a live game's commands are, with any that cannot currently be used shown as unavailable rather than silently doing nothing.

Help within a replay SHALL describe the replay's keys, so that the screen never advertises a key that does nothing here.

#### Scenario: Commands shown
- **WHEN** a replay is on screen
- **THEN** the flip, step, history, help and quit keys are visible with their meanings

#### Scenario: Unavailable step
- **WHEN** the replay is at the start or the end of the game
- **THEN** the step that would go past that end is shown as unavailable

#### Scenario: Help describes the replay
- **WHEN** the player asks for help during a replay
- **THEN** it covers the replay's own keys, and dismissing it returns to the same position

### Requirement: The end of the game is stated, not screened

When the replay is at the final position of the game, it SHALL state the result and the reason — who won and how, or that it was a draw and why — on the board screen itself.

A replay SHALL NOT show the game-result screen. Ending the replay at its last move would make the final position a place the player cannot step back out of, which is exactly the position most worth examining.

#### Scenario: Result at the end
- **WHEN** the replay reaches the final position
- **THEN** the result and its reason are stated on screen, and the board remains steppable

#### Scenario: Stepping back off the end
- **WHEN** the player steps backward from the final position
- **THEN** the previous position is shown and the result is no longer stated as current

#### Scenario: No result screen
- **WHEN** the replay reaches the final position by any route
- **THEN** the game-result screen does not appear, and nothing takes the replay off the board

#### Scenario: A result the moves do not reveal
- **WHEN** the replayed game ended by resignation or by an agreed draw
- **THEN** the stated result names that reason, rather than describing the position as merely unfinished

### Requirement: Every view follows the step

Stepping SHALL update everything the screen shows about the game together: the board, the captured pieces, the check indicator, the last-move marking, and the side to move.

#### Scenario: Views agree
- **WHEN** the player steps backward over a capture
- **THEN** the captured piece is back on the board and out of the captured list
- **AND** the last-move marking shows the move before it, or nothing at the starting position

#### Scenario: Check reappears
- **WHEN** the player steps back to a position in which a king was in check
- **THEN** the check indicator is shown again, as it was during the game

#### Scenario: Side to move
- **WHEN** the player steps to any position
- **THEN** the side that was to move in that position is stated, whichever way the board is facing

### Requirement: Leaving a replay

Quitting a replay SHALL leave it directly, without asking whether to save: nothing in a replay can be lost, so there is nothing to confirm.

#### Scenario: Quitting
- **WHEN** the player quits a replay
- **THEN** it closes with no prompt, and the saved game is left as it was

#### Scenario: Reopening
- **WHEN** the player opens the same finished game again
- **THEN** it opens at the final position, as it did the first time, regardless of where the previous replay was left
