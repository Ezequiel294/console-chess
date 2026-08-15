## 1. Position model

- [x] 1.1 Define `Position` in `position.h`: board, side to move, castling rights (4 bits), en passant square, halfmove clock, fullmove number
- [x] 1.2 Keep the position small enough to copy by value — the legality filter copies it once per candidate move
- [x] 1.3 `position_init()` for the standard starting position
- [x] 1.4 Square helpers: index to file/rank, bounds checks, colour of a square

## 2. FEN

Built before generation so every later test can be written as a one-line fixture.

- [x] 2.1 `fen_parse()` — all six fields into a `Position`
- [x] 2.2 `fen_write()` — a `Position` into a string
- [x] 2.3 Reject malformed input: wrong field count, bad rank lengths, unknown piece letters, missing king
- [x] 2.4 Round-trip test over a set of positions, asserting every component survives

## 3. Move representation and application

- [x] 3.1 Define `Move`: from, to, moved piece, captured piece, promotion piece, flags for castling / en passant / double push
- [x] 3.2 Have `Move` carry displaced state — captured piece, previous castling rights, previous en passant square, previous halfmove clock
- [x] 3.3 `make(pos, move)` — move the piece, handle the rook in castling, remove the pawn in en passant, place the promoted piece, update rights, set or clear the en passant square, update both clocks, flip the side to move
- [x] 3.4 `unmake(pos, move)` — restore **all** of the above, not just piece placement
- [x] 3.5 Test make-then-unmake over every legal move from several positions, asserting byte-identical restoration

## 4. Attack detection

- [x] 4.1 `is_square_attacked(pos, square, by_side)` — radiate outward from the square rather than generating all enemy moves
- [x] 4.2 Cover sliders along their lines with blocking, knights at their offsets, pawns diagonally, and the adjacent king
- [x] 4.3 `in_check(pos, side)` built on it
- [x] 4.4 Test against hand-built positions covering each attacker type, including blocked lines

## 5. Pseudo-legal generation

- [x] 5.1 Port the geometry from `rules.c` into per-piece generators
- [x] 5.2 Sliders: rook, bishop, queen, stopping at the first occupied square and including it if it holds an enemy
- [x] 5.3 Knight and king offsets with bounds checks
- [x] 5.4 Pawns: single advance, double advance from the starting rank with both squares empty, diagonal captures only
- [x] 5.5 Exclude destinations occupied by a piece of the same colour
- [x] 5.6 Delete `rules.c`

## 6. Legality filter

- [x] 6.1 `generate_legal_moves(pos, out)` — for each pseudo-legal move, apply to a scratch copy and reject if the mover's king is attacked
- [x] 6.2 `generate_legal_moves_from(pos, square, out)` for a single piece — the entry point `mouse-and-highlights` will use
- [x] 6.3 Verify the initial position yields exactly 20 moves
- [x] 6.4 Verify a pinned piece's moves are restricted, and that a king never has a move onto an attacked square

## 7. Special moves

- [x] 7.1 Castling generation with all five conditions: rights intact, squares between empty, not currently in check, not passing through an attacked square, not landing in check
- [x] 7.2 Revoke rights on king move, on rook move from its starting square, and on rook capture on its starting square
- [x] 7.3 En passant: set the target on a double push, clear it after one ply, generate the diagonal capture, remove the captured pawn from its own square
- [x] 7.4 Handle the en passant case where both pawns leaving the rank would expose the king — the classic omission
- [x] 7.5 Promotion: emit four moves per promotion square, for both advances and captures
- [x] 7.6 Verify castling is legal when the rook, but not the king's path, is attacked

## 8. Perft

- [x] 8.1 `perft(pos, depth)` counting leaf nodes
- [x] 8.2 Divided perft — per-move subtree counts — for bisecting a mismatch
- [x] 8.3 Initial position fixture: 20, 400, 8902, 197281, 4865609
- [x] 8.4 "Kiwipete" fixture for castling and en passant
- [x] 8.5 Fixtures for promotion-with-check and the en passant pin
- [x] 8.6 `make test` running to depth 4; `make test-full` for depth 5 and above
- [x] 8.7 **All published counts must match exactly before this change is considered done**

## 9. Game outcome

- [x] 9.1 `outcome(pos)` returning in-progress, checkmate, stalemate, or a draw with its reason
- [x] 9.2 Checkmate: in check and no legal move
- [x] 9.3 Stalemate: not in check and no legal move
- [x] 9.4 Fifty-move rule from the halfmove clock
- [x] 9.5 Insufficient material: K/K, K+B/K, K+N/K, K+B/K+B with same-coloured bishops
- [x] 9.6 Zobrist hashing from a fixed seed table, updated incrementally in make/unmake
- [x] 9.7 Threefold repetition over the position history
- [x] 9.8 Remove `captured_king` and every reference to it
- [x] 9.9 Test each termination against a constructed position

## 10. Notation

- [x] 10.1 Coordinate move text: write and parse, including the promotion suffix
- [x] 10.2 Resolve coordinate text against a position's legal moves, rejecting text with no match
- [x] 10.3 SAN output: piece letter, capture marker, destination, promotion, castling, check and checkmate markers
- [x] 10.4 SAN disambiguation by file, then by rank when files match
- [x] 10.5 Test SAN against a recorded game with known notation

## 11. Integration

- [x] 11.1 Switch `game.c` from `moves % 2` to `position.side_to_move`
- [x] 11.2 Replace the king-capture end condition with `outcome()`
- [x] 11.3 `Promotion` overlay screen offering the four pieces — the first real overlay, and a check that the screen stack works
- [x] 11.4 Show a check indicator on the game screen
- [x] 11.5 Reject moves not present in the legal move list
- [x] 11.6 Read and write FEN in the save path in place of the old binary format; reject older files
- [x] 11.7 Play a full game to checkmate, one to stalemate, and one exercising castling, en passant, and underpromotion
