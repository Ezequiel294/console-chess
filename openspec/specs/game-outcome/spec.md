# game-outcome Specification

## Purpose

Determines when a game has ended and why — replacing king capture with the conditions chess actually recognizes, so that a game concludes at checkmate, stalemate, or a draw rather than at the removal of a king.

## Requirements

### Requirement: Check detection

The system SHALL report whether the side to move is in check, meaning its king stands on a square the opponent attacks.

#### Scenario: King attacked
- **WHEN** an enemy piece attacks the square holding the king of the side to move
- **THEN** that side is reported to be in check

#### Scenario: Attack is blocked
- **WHEN** a friendly or enemy piece stands between the king and an enemy slider
- **THEN** the king is not in check

#### Scenario: Discovered check
- **WHEN** a move vacates a line, exposing the enemy king to a slider behind it
- **THEN** the opponent is in check after that move

### Requirement: Checkmate

The game SHALL end in checkmate when the side to move is in check and has no legal move. The side delivering the check wins.

#### Scenario: Checkmate reached
- **WHEN** the side to move is in check and every legal move has been exhausted with none available
- **THEN** the game ends and the opposing side is declared the winner

#### Scenario: Check with an escape
- **WHEN** the side to move is in check but has at least one legal move
- **THEN** the game continues

#### Scenario: Check answered by capture or block
- **WHEN** the checking piece can be captured, or the check can be blocked
- **THEN** those moves are available and the position is not checkmate

### Requirement: Kings are never captured

A king SHALL NOT be capturable. No legal move may result in a king's removal from the board, and the game SHALL NOT end by king capture.

This replaces the previous win condition.

#### Scenario: No move exposes a king to capture
- **WHEN** any legal position is reached
- **THEN** the side that just moved is not in check, so its king cannot be taken

#### Scenario: Game ends before capture
- **WHEN** a king has no escape from attack
- **THEN** the game ends at checkmate, with both kings still on the board

### Requirement: Stalemate

The game SHALL end in a draw by stalemate when the side to move is not in check and has no legal move.

#### Scenario: Stalemate reached
- **WHEN** the side to move is not in check and has no legal move
- **THEN** the game ends in a draw, recorded as stalemate

#### Scenario: Stalemate is not a loss
- **WHEN** stalemate occurs
- **THEN** neither side is declared the winner, regardless of material

### Requirement: Draw by fifty-move rule

The game SHALL be drawable when fifty moves by each side have passed with no capture and no pawn move. The counter SHALL reset on any capture or pawn move.

#### Scenario: Threshold reached
- **WHEN** one hundred consecutive half-moves occur with no capture and no pawn move
- **THEN** the game may be drawn under the fifty-move rule

#### Scenario: Counter resets
- **WHEN** a capture or a pawn move occurs
- **THEN** the count restarts from zero

### Requirement: Draw by insufficient material

The game SHALL end in a draw when neither side possesses material sufficient to deliver checkmate: king against king, king and bishop against king, king and knight against king, and king and bishop against king and bishop with both bishops on squares of one color.

#### Scenario: Bare kings
- **WHEN** only the two kings remain
- **THEN** the game ends in a draw immediately

#### Scenario: Lone minor piece
- **WHEN** one side has only a king and a single bishop or knight, and the other only a king
- **THEN** the game ends in a draw

#### Scenario: Sufficient material remains
- **WHEN** either side retains a pawn, rook, or queen
- **THEN** the game continues

### Requirement: Draw by threefold repetition

The game SHALL be drawable when the same position occurs three times with the same side to move, the same castling rights, and the same en passant possibilities.

#### Scenario: Position repeated three times
- **WHEN** an identical position arises for the third time
- **THEN** the game may be drawn by repetition

#### Scenario: Rights differ
- **WHEN** piece placement repeats but castling rights have changed
- **THEN** the positions are not the same and do not count toward repetition

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
