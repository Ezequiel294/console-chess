## MODIFIED Requirements

### Requirement: Game result

When a game ends, the system SHALL display the result, the reason, and the final position, and SHALL offer saving the finished game, a new game, reviewing the move history, and returning to the main menu.

Saving is offered here because this is the only screen a finished game is ever seen from, and leaving it without saving discards the game for good. It is offered rather than performed: keeping a game is the player's decision, the same as it is during play (see game-persistence's Saving is explicit).

#### Scenario: Result shown
- **WHEN** a game ends by any means
- **THEN** the winner or draw, the reason, and the final position are displayed

#### Scenario: Saving the finished game
- **WHEN** the player chooses to save from the result screen
- **THEN** the finished game is written to disk — asking for a name if it has never been saved — and the result screen remains, reporting what happened

#### Scenario: Saving a game that was already saved
- **WHEN** the player saves a game from the result screen that had been saved while in progress
- **THEN** it is not asked to be named again, and the finished save replaces the in-progress one

#### Scenario: Leaving without saving
- **WHEN** the player leaves the result screen without saving
- **THEN** nothing is written, and no prompt asks them to reconsider

#### Scenario: Board orientation is continuous
- **WHEN** a game ends
- **THEN** the final position is shown in the same orientation the player was already looking at, not one recomputed from whose turn it is — nothing about ending the game looks like the board flipped on its own

#### Scenario: Checkmate marks the losing king
- **WHEN** a game ends in checkmate
- **THEN** the checked king's square is marked the same way it is during play, so the result screen answers "which king" as well as "who won"

#### Scenario: Review from the result
- **WHEN** the player chooses to review from the result screen
- **THEN** the full move history is available

#### Scenario: No further moves
- **WHEN** a game has ended
- **THEN** no move can be made in it
