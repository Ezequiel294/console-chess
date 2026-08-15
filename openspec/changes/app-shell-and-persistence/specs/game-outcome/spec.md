## ADDED Requirements

### Requirement: Resignation

A player SHALL be able to resign, ending the game immediately with the opponent as the winner. Resignation SHALL require confirmation.

#### Scenario: Resigning
- **WHEN** a player resigns and confirms
- **THEN** the game ends immediately and the opponent is the winner

#### Scenario: Resignation cancelled
- **WHEN** a player begins to resign and cancels the confirmation
- **THEN** the game continues unchanged, with the same side to move

#### Scenario: Resignation is available on either turn
- **WHEN** either player resigns, whether or not it is their turn
- **THEN** the game ends with the other player as the winner

### Requirement: Draw by agreement

A player SHALL be able to offer a draw. The opponent SHALL be able to accept, ending the game in a draw, or decline, leaving the game unchanged.

An offer SHALL stand only until the opponent responds.

#### Scenario: Offer accepted
- **WHEN** a draw is offered and the opponent accepts
- **THEN** the game ends in a draw by agreement

#### Scenario: Offer declined
- **WHEN** a draw is offered and the opponent declines
- **THEN** the game continues unchanged, with the same side to move

#### Scenario: Offer does not persist
- **WHEN** a draw is offered and declined
- **THEN** it cannot be accepted later without being offered again

## MODIFIED Requirements

### Requirement: Game termination reasons

Every concluded game SHALL carry a reason: checkmate, stalemate, fifty-move rule, insufficient material, threefold repetition, resignation, or draw by agreement. The result SHALL identify the winner, or record a draw.

#### Scenario: Reason reported
- **WHEN** a game ends by any means
- **THEN** the outcome states both the result and the reason for it

#### Scenario: Play stops
- **WHEN** a game has ended
- **THEN** no further moves are accepted

#### Scenario: Player-chosen termination
- **WHEN** a game ends by resignation or by agreed draw
- **THEN** the reason recorded distinguishes it from a termination forced by the rules
