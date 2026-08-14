## 1. Position model

- [ ] 1.1 Define `Position` in `position.h`: board, side to move, castling rights (4 bits), en passant square, halfmove clock, fullmove number
- [ ] 1.2 Keep the position small enough to copy by value — the legality filter copies it once per candidate move
- [ ] 1.3 `position_init()` for the standard starting position
- [ ] 1.4 Square helpers: index to file/rank, bounds checks, colour of a square

## 2. FEN

Built before generation so every later test can be written as a one-line fixture.

- [ ] 2.1 `fen_parse()` — all six fields into a `Position`
- [ ] 2.2 `fen_write()` — a `Position` into a string
- [ ] 2.3 Reject malformed input: wrong field count, bad rank lengths, unknown piece letters, missing king
- [ ] 2.4 Round-trip test over a set of positions, asserting every component survives

## 3. Move representation and application

- [ ] 3.1 Define `Move`: from, to, moved piece, captured piece, promotion piece, flags for castling / en passant / double push
- [ ] 3.2 Have `Move` carry displaced state — captured piece, previous castling rights, previous en passant square, previous halfmove clock
- [ ] 3.3 `make(pos, move)` — move the piece, handle the rook in castling, remove the pawn in en passant, place the promoted piece, update rights, set or clear the en passant square, update both clocks, flip the side to move
- [ ] 3.4 `unmake(pos, move)` — restore **all** of the above, not just piece placement
- [ ] 3.5 Test make-then-unmake over every legal move from several positions, asserting byte-identical restoration

## 4. Attack detection

- [ ] 4.1 `is_square_attacked(pos, square, by_side)` — radiate outward from the square rather than generating all enemy moves
- [ ] 4.2 Cover sliders along their lines with blocking, knights at their offsets, pawns diagonally, and the adjacent king
- [ ] 4.3 `in_check(pos, side)` built on it
- [ ] 4.4 Test against hand-built positions covering each attacker type, including blocked lines

## 5. Pseudo-legal generation

- [ ] 5.1 Port the geometry from `rules.c` into per-piece generators
- [ ] 5.2 Sliders: rook, bishop, queen, stopping at the first occupied square and including it if it holds an enemy
- [ ] 5.3 Knight and king offsets with bounds checks
- [ ] 5.4 Pawns: single advance, double advance from the starting rank with both squares empty, diagonal captures only
- [ ] 5.5 Exclude destinations occupied by a piece of the same colour
- [ ] 5.6 Delete `rules.c`

## 6. Legality filter

- [ ] 6.1 `generate_legal_moves(pos, out)` — for each pseudo-legal move, apply to a scratch copy and reject if the mover's king is attacked
- [ ] 6.2 `generate_legal_moves_from(pos, square, out)` for a single piece — the entry point `mouse-and-highlights` will use
- [ ] 6.3 Verify the initial position yields exactly 20 moves
- [ ] 6.4 Verify a pinned piece's moves are restricted, and that a king never has a move onto an attacked square

## 7. Special moves

- [ ] 7.1 Castling generation with all five conditions: rights intact, squares between empty, not currently in check, not passing through an attacked square, not landing in check
- [ ] 7.2 Revoke rights on king move, on rook move from its starting square, and on rook capture on its starting square
- [ ] 7.3 En passant: set the target on a double push, clear it after one ply, generate the diagonal capture, remove the captured pawn from its own square
- [ ] 7.4 Handle the en passant case where both pawns leaving the rank would expose the king — the classic omission
- [ ] 7.5 Promotion: emit four moves per promotion square, for both advances and captures
- [ ] 7.6 Verify castling is legal when the rook, but not the king's path, is attacked

## 8. Perft

- [ ] 8.1 `perft(pos, depth)` counting leaf nodes
- [ ] 8.2 Divided perft — per-move subtree counts — for bisecting a mismatch
- [ ] 8.3 Initial position fixture: 20, 400, 8902, 197281, 4865609
- [ ] 8.4 "Kiwipete" fixture for castling and en passant
- [ ] 8.5 Fixtures for promotion-with-check and the en passant pin
- [ ] 8.6 `make test` running to depth 4; `make test-full` for depth 5 and above
- [ ] 8.7 **All published counts must match exactly before this change is considered done**

## 9. Game outcome

- [ ] 9.1 `outcome(pos)` returning in-progress, checkmate, stalemate, or a draw with its reason
- [ ] 9.2 Checkmate: in check and no legal move
- [ ] 9.3 Stalemate: not in check and no legal move
- [ ] 9.4 Fifty-move rule from the halfmove clock
- [ ] 9.5 Insufficient material: K/K, K+B/K, K+N/K, K+B/K+B with same-coloured bishops
- [ ] 9.6 Zobrist hashing from a fixed seed table, updated incrementally in make/unmake
- [ ] 9.7 Threefold repetition over the position history
- [ ] 9.8 Remove `captured_king` and every reference to it
- [ ] 9.9 Test each termination against a constructed position

## 10. Notation

- [ ] 10.1 Coordinate move text: write and parse, including the promotion suffix
- [ ] 10.2 Resolve coordinate text against a position's legal moves, rejecting text with no match
- [ ] 10.3 SAN output: piece letter, capture marker, destination, promotion, castling, check and checkmate markers
- [ ] 10.4 SAN disambiguation by file, then by rank when files match
- [ ] 10.5 Test SAN against a recorded game with known notation

## 11. Integration

- [ ] 11.1 Switch `game.c` from `moves % 2` to `position.side_to_move`
- [ ] 11.2 Replace the king-capture end condition with `outcome()`
- [ ] 11.3 `Promotion` overlay screen offering the four pieces — the first real overlay, and a check that the screen stack works
- [ ] 11.4 Show a check indicator on the game screen
- [ ] 11.5 Reject moves not present in the legal move list
- [ ] 11.6 Read and write FEN in the save path in place of the old binary format; reject older files
- [ ] 11.7 Play a full game to checkmate, one to stalemate, and one exercising castling, en passant, and underpromotion
