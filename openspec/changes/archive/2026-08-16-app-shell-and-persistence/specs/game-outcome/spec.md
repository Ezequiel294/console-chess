## ADDED Requirements

### Requirement: Resignation

A player SHALL be able to resign, ending the game immediately with the opponent as the winner. Resignation SHALL require confirmation.

The side to move is the side that resigns; the confirmation SHALL ask only whether they are sure, and SHALL name who the win goes to. It SHALL NOT ask which player is resigning.

An earlier version did ask, reasoning that pass-and-play shares one keyboard and `side_to_move` changes only on an actual move, so the program cannot tell the two players apart and must not assume. The reasoning holds and the consequence is real: resigning strictly out of turn is no longer expressible. It is the right trade anyway — the turn handover already establishes whose turn it is before either player touches a key, and making every resignation answer "who" to preserve a case that does not arise in practice cost more than it saved.

#### Scenario: Resigning
- **WHEN** the side to move resigns and confirms
- **THEN** the game ends immediately and the opponent is the winner

#### Scenario: The confirmation names the stakes
- **WHEN** the resignation confirmation is shown
- **THEN** it states which side the win goes to, rather than asking which side is resigning

#### Scenario: Resignation cancelled
- **WHEN** a player begins to resign and cancels the confirmation
- **THEN** the game continues unchanged, with the same side to move

### Requirement: Draw by agreement

A player SHALL be able to offer a draw. The opponent SHALL be able to accept, ending the game in a draw, or decline, leaving the game unchanged.

An offer SHALL stand only until the opponent responds.

Offering and responding SHALL be one exchange, not two separate commands: the offer command opens the opponent's accept/decline response immediately. Pass-and-play shares one keyboard, so a two-step version — press once to offer, press again to answer — cannot distinguish the two players from each other, and nothing on screen can tell the offerer that a second press is what comes next; the first press simply looks as though it failed. The response names both sides explicitly instead, so whoever is holding the keyboard knows which of them it is addressed to.

#### Scenario: Offering opens the response
- **WHEN** a player issues the draw-offer command
- **THEN** the opponent's accept-or-decline response appears immediately, naming who offered and who is answering

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
