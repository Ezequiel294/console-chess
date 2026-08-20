## MODIFIED Requirements

### Requirement: Commands replace prompting

In-game actions SHALL be invoked by the player through single-key commands. The system SHALL NOT interrupt play to ask questions the player did not initiate.

The available commands during a live game SHALL be: save, view history, resign, offer a draw, help, and quit.

There is deliberately no command to flip the board *during a live game*. The turn handover already turns it, and a manual flip only lets the side to move study the position from their opponent's seat — which is not something either player is entitled to mid-game. See Turn handover. A replay is the other case entirely: there is no turn, no handover, and no opponent to gain an advantage over, so a replay does have a flip command (see game-replay).

Undo and redo are likewise not live-play commands: chess does not allow taking back a move already made. They are reachable only in a replay of a finished game, where stepping backward changes nothing that was played (see move-undo and game-replay).

#### Scenario: Command issued
- **WHEN** the player presses a command key during their turn
- **THEN** that action is taken without disturbing the position

#### Scenario: No unsolicited prompts
- **WHEN** a game runs for any number of moves
- **THEN** no prompt appears that the player did not trigger

#### Scenario: Command during opponent's turn
- **WHEN** a command that does not alter the position is issued at any point in a turn
- **THEN** it works, and the side to move is unchanged afterwards

#### Scenario: No manual flip
- **WHEN** the player looks for a way to turn the board mid-turn in a live game
- **THEN** there is none; the board's orientation changes only at the handover

#### Scenario: No undo in a live game
- **WHEN** the player presses the undo or redo key during a live game
- **THEN** nothing happens, and neither is listed among the live game's commands
