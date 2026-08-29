## MODIFIED Requirements

### Requirement: History reflects undone moves

In a live game, when moves are undone the history SHALL no longer list them as played.

In a replay this is inverted: the history SHALL list every move of the finished game, including the ones the replay has stepped back past, and SHALL mark the move the board is currently showing. A replay is a review of a game whose moves are all already known — withholding the ones ahead would hide the very thing being reviewed, and would leave the history shrinking and growing as the player steps, which reads as damage rather than as position.

#### Scenario: After undo
- **WHEN** a move is undone in a live game and the history is opened
- **THEN** that move is not listed among the moves played

#### Scenario: Replay lists the whole game
- **WHEN** the history is opened during a replay, at any position
- **THEN** every move of the game is listed, including those after the current position

#### Scenario: The current move is marked
- **WHEN** the history is opened during a replay
- **THEN** the move the board is currently showing is marked among the rest, distinguishable at a glance

#### Scenario: The mark follows the board
- **WHEN** the player steps to another move and opens the history again
- **THEN** the mark is on that move

#### Scenario: At the starting position
- **WHEN** the history is opened during a replay stepped all the way back to the starting position
- **THEN** every move is still listed, and no move is marked as current
