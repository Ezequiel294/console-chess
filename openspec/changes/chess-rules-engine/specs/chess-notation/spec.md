## Purpose

Reads and writes positions and moves in the standard chess text formats, so that positions can be exchanged with other chess software, written directly into tests as one-line fixtures, and stored in save files that a person can read.

## ADDED Requirements

### Requirement: Position text format

The system SHALL read and write a complete position as a single line of text in the standard field order: piece placement, side to move, castling rights, en passant target, halfmove clock, fullmove number.

Writing a position and reading it back SHALL yield an identical position.

#### Scenario: Round trip
- **WHEN** any position is written to text and read back
- **THEN** the resulting position is identical in placement, side to move, castling rights, en passant target, and both clocks

#### Scenario: Initial position
- **WHEN** the initial position is written
- **THEN** the output is the standard initial position string

#### Scenario: Position from external software
- **WHEN** a position string produced by other chess software is read
- **THEN** it loads correctly and play may continue from it

#### Scenario: Malformed input rejected
- **WHEN** a string is not a valid position — wrong field count, bad rank lengths, an unknown piece letter, or a missing king
- **THEN** it is rejected with an indication of the problem, and no partially loaded position is produced

### Requirement: Move text format

The system SHALL read and write moves in coordinate form: origin square, destination square, and for a promotion a trailing piece letter.

#### Scenario: Ordinary move
- **WHEN** a move from e2 to e4 is written
- **THEN** the output is `e2e4`

#### Scenario: Promotion
- **WHEN** a pawn promotes to a knight on e8
- **THEN** the output is `e7e8n`

#### Scenario: Reading a move
- **WHEN** coordinate move text is read against a position
- **THEN** it resolves to the matching legal move, or is rejected if no legal move matches

#### Scenario: Castling in coordinate form
- **WHEN** a castling move is written
- **THEN** it is expressed as the king's origin and destination squares

### Requirement: Readable move notation

The system SHALL produce standard algebraic notation for a move in the context of its position, for display in the move list.

Notation SHALL include the piece letter, the capture marker, the destination, promotion, castling, and the check or checkmate marker, and SHALL disambiguate by file or rank when two identical pieces could reach the same square.

#### Scenario: Pawn advance
- **WHEN** a pawn moves from e2 to e4
- **THEN** the notation is `e4`

#### Scenario: Piece capture
- **WHEN** a knight captures on f3
- **THEN** the notation is `Nxf3`

#### Scenario: Castling
- **WHEN** a player castles kingside or queenside
- **THEN** the notation is `O-O` or `O-O-O`

#### Scenario: Promotion with check
- **WHEN** a pawn promotes to a queen on e8 and gives check
- **THEN** the notation is `e8=Q+`

#### Scenario: Checkmate marker
- **WHEN** a move delivers checkmate
- **THEN** the notation ends in `#`

#### Scenario: Two pieces could reach the square
- **WHEN** two knights can both reach d2
- **THEN** the notation identifies which one moved by file, or by rank when the files match
