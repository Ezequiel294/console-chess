## Purpose

Defines what a legal chess position is and which moves are legal from it, including the three special moves the game currently lacks, so that every other part of the system can ask "what can this piece do?" and get the answer chess gives.

## ADDED Requirements

### Requirement: Complete position state

A position SHALL carry everything required to determine legality: the placement of pieces, the side to move, castling rights for each side and wing, the en passant target square if one exists, the halfmove clock since the last capture or pawn move, and the fullmove number.

Piece placement alone is insufficient — castling and en passant depend on history.

#### Scenario: Two identical arrangements, different rights
- **WHEN** two positions have identical piece placement but differ in castling rights
- **THEN** they are different positions and may permit different moves

#### Scenario: En passant expires
- **WHEN** a pawn advances two squares, creating an en passant target
- **AND** the opponent makes any move other than capturing en passant
- **THEN** the target is cleared and the capture is no longer available

### Requirement: Legal move generation

The system SHALL produce, for a given position, the complete set of legal moves — every move chess permits, and no move it forbids.

A move is legal only if it does not leave the mover's own king attacked. This applies to every piece, including pinned pieces and the king itself.

#### Scenario: Starting position
- **WHEN** legal moves are generated for the initial position
- **THEN** exactly 20 moves are produced

#### Scenario: Pinned piece
- **WHEN** a piece stands between its own king and an attacking slider
- **THEN** its generated moves are limited to those that keep the king shielded

#### Scenario: King may not move into attack
- **WHEN** legal moves are generated for a king
- **THEN** no generated move places it on a square the opponent attacks

#### Scenario: Response to check is forced
- **WHEN** the side to move is in check
- **THEN** every generated move leaves that side out of check

#### Scenario: Own pieces are not captured
- **WHEN** moves are generated for any piece
- **THEN** no generated move lands on a square occupied by a piece of the same color

### Requirement: Piece movement

Each piece SHALL move as chess defines. Sliding pieces SHALL NOT pass through occupied squares; knights SHALL be unobstructed.

Pawns SHALL advance one square, or two from their starting rank when both squares are empty, SHALL capture only diagonally forward, and SHALL NOT capture straight ahead.

#### Scenario: Blocked slider
- **WHEN** a rook, bishop, or queen has a piece between it and a target square
- **THEN** that target and everything beyond it is unreachable

#### Scenario: Knight jumps
- **WHEN** a knight is surrounded by pieces
- **THEN** its L-shaped destinations remain available

#### Scenario: Pawn blocked ahead
- **WHEN** the square directly in front of a pawn is occupied by any piece
- **THEN** the pawn cannot advance to it, regardless of that piece's color

#### Scenario: Double advance requires both squares clear
- **WHEN** a pawn on its starting rank has an occupied square immediately ahead
- **THEN** the two-square advance is unavailable

### Requirement: Castling

Castling SHALL be generated as a king move of two squares toward a rook, permitted only when all of the following hold: the relevant castling right is intact, every square between king and rook is empty, the king is not currently in check, the king does not pass through an attacked square, and the king does not land in check.

Castling rights SHALL be revoked permanently when the king moves, when a rook moves from its starting square, or when a rook is captured on its starting square.

#### Scenario: Kingside castling available
- **WHEN** rights are intact, the squares between are empty, and neither the king's origin, path, nor destination is attacked
- **THEN** castling appears among the legal moves

#### Scenario: Castling out of check
- **WHEN** the king is in check
- **THEN** no castling move is generated on either wing

#### Scenario: Castling through an attacked square
- **WHEN** the square the king would cross is attacked
- **THEN** castling on that wing is not generated, even though the destination is safe

#### Scenario: Rights lost by moving the king
- **WHEN** the king moves and later returns to its starting square
- **THEN** castling remains unavailable on both wings

#### Scenario: Rights lost by rook capture
- **WHEN** a rook is captured on its starting square
- **THEN** castling on that wing becomes unavailable

#### Scenario: Rook may be attacked
- **WHEN** the rook's square or the square it passes over is attacked, but the king's origin, path, and destination are not
- **THEN** castling is legal

### Requirement: En passant

When a pawn advances two squares past an adjacent enemy pawn, that enemy pawn SHALL be able to capture it on the immediately following move by moving diagonally to the skipped square, removing the captured pawn from the square it occupies.

This capture SHALL be available only on the move immediately following the double advance.

#### Scenario: Capture available
- **WHEN** an enemy pawn advances two squares to a square beside a friendly pawn
- **THEN** the diagonal capture to the skipped square is a legal move

#### Scenario: Opportunity expires
- **WHEN** the capture is not taken on the immediately following move
- **THEN** it is no longer available

#### Scenario: Captured pawn is removed
- **WHEN** an en passant capture is made
- **THEN** the captured pawn is removed from the board, though it stood on a different square from the capturing pawn's destination

#### Scenario: Capture would expose the king
- **WHEN** an en passant capture would leave the mover's king attacked along the rank, because both the capturing and captured pawn leave that rank
- **THEN** the capture is not legal

### Requirement: Pawn promotion

A pawn reaching the far rank SHALL be promoted. Generation SHALL produce a distinct move for each of queen, rook, bishop, and knight. A pawn SHALL NOT remain a pawn on the far rank.

#### Scenario: Four promotion choices
- **WHEN** a pawn can reach the far rank
- **THEN** four legal moves to that square are generated, one per promotion piece

#### Scenario: Promotion by capture
- **WHEN** a pawn can capture onto the far rank
- **THEN** promotion choices are generated for that capture as well

#### Scenario: Underpromotion available
- **WHEN** a player promotes
- **THEN** knight, bishop, and rook are selectable, not queen alone

#### Scenario: Promotion delivering check
- **WHEN** a promotion would deliver check or checkmate
- **THEN** it is generated and evaluated as any other move

### Requirement: Move application is reversible

Applying a move SHALL be exactly reversible, restoring the previous position in full — piece placement, castling rights, en passant target, and both clocks.

#### Scenario: Apply and reverse
- **WHEN** any legal move is applied and then reversed
- **THEN** the position is identical to before in every component

#### Scenario: Reversing a castling move
- **WHEN** a castling move is reversed
- **THEN** both king and rook return to their squares and the rights are restored

#### Scenario: Reversing an en passant capture
- **WHEN** an en passant capture is reversed
- **THEN** the captured pawn returns to its own square, not the capturing pawn's destination

#### Scenario: Reversing a promotion
- **WHEN** a promotion is reversed
- **THEN** a pawn is restored, not the promoted piece

### Requirement: Generated move correctness is verifiable

The system SHALL provide a way to count the leaf nodes of the move tree to a given depth from a given position, so that generation can be checked against the published reference counts for standard test positions.

#### Scenario: Initial position counts
- **WHEN** leaf nodes are counted from the initial position
- **THEN** the results are 20, 400, 8902, 197281, and 4865609 for depths 1 through 5

#### Scenario: Castling and en passant test position
- **WHEN** leaf nodes are counted from the standard position designed to exercise castling and en passant
- **THEN** the results match the published reference counts at every depth tested
