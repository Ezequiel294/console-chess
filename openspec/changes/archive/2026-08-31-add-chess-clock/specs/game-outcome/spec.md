## MODIFIED Requirements

### Requirement: Game termination reasons

Every concluded game SHALL carry a reason: checkmate, stalemate, fifty-move rule, insufficient material, threefold repetition, resignation, draw by agreement, running out of time, or running out of time against an opponent who could not have mated. The result SHALL identify the winner, or record a draw.

The last two are conclusions the clock forces rather than the position: a game that ends on time ends in a position that is, on the board alone, merely unfinished. A concluded game SHALL therefore be distinguishable as having ended on time, and SHALL NOT be described as though the position ended it.

#### Scenario: Reason reported
- **WHEN** a game ends by any means
- **THEN** the outcome states both the result and the reason for it

#### Scenario: Play stops
- **WHEN** a game has ended
- **THEN** no further moves are accepted

#### Scenario: Player-chosen termination
- **WHEN** a game ends by resignation or by agreed draw
- **THEN** the reason recorded distinguishes it from a termination forced by the rules

#### Scenario: Termination on time
- **WHEN** a game ends because a side's clock reached zero
- **THEN** the reason recorded names running out of time, and the opponent is recorded as the winner

#### Scenario: Termination on time with nothing to mate with
- **WHEN** a game ends because a side's clock reached zero and the opponent could not deliver checkmate with the material they hold
- **THEN** the reason recorded names that case specifically, and the result is a draw

## ADDED Requirements

### Requirement: Material sufficient to mate

The system SHALL be able to determine whether a side holds material with which checkmate could be delivered at all, independently of whether the game has ended.

A lone king, a king and a single bishop, and a king and a single knight SHALL be judged unable to mate. Anything else — including a king with two knights, which cannot force mate but can reach one — SHALL be judged able to mate.

This is a question about one side, and is distinct from the draw by insufficient material, which is a question about both sides at once.

#### Scenario: Bare king
- **WHEN** a side holds only its king
- **THEN** it is judged unable to mate

#### Scenario: King and minor piece
- **WHEN** a side holds its king and a single bishop, or its king and a single knight
- **THEN** it is judged unable to mate

#### Scenario: King and two knights
- **WHEN** a side holds its king and two knights
- **THEN** it is judged able to mate, since a helpmate exists even though mate cannot be forced

#### Scenario: Any major piece or pawn
- **WHEN** a side holds a queen, a rook, or a pawn
- **THEN** it is judged able to mate

#### Scenario: Asymmetry
- **WHEN** one side holds a lone king and the other holds a queen
- **THEN** the first is judged unable to mate and the second able, even though this is not a position that draws by insufficient material
