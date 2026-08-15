#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.h"

/* Move application and generation.
 *
 * Two layers: pseudo-legal generation knows piece geometry only, and
 * generate_legal_moves() filters it by applying each candidate to a scratch
 * position and rejecting it if the mover's king ends up attacked. That single
 * filter is what delivers pins, check evasion, and "a king may not move into
 * check" for every piece, with no per-piece special-casing.
 *
 * This layer performs no I/O. It takes positions and returns values.
 */

/* Safely above the ~218 legal moves possible in any real position. */
#define MAX_MOVES 256

typedef struct {
  Move moves[MAX_MOVES];
  int count;
} MoveList;

/* Applies move to *pos in place: moves the piece, handles the rook in
 * castling, removes the pawn taken en passant, places a promoted piece,
 * updates castling rights, sets or clears the en passant square, updates both
 * clocks, and flips the side to move. move must be pseudo-legal in *pos. */
void make(Position *pos, Move move);

/* Reverses exactly what make() did, using the state move carries. Calling
 * unmake(pos, move) on the position make(pos, move) produced restores it to
 * the position before, in every field, including the hash. */
void unmake(Position *pos, Move move);

/* Whether by_side attacks (i, j) in the current position — a piece of that
 * side could move to (i, j) and capture whatever stands there (or, if it is
 * empty, could a piece land there). Radiates outward from the square rather
 * than generating every move by_side has. Used as the legality filter, for
 * check detection, and for castling's "may not pass through an attacked
 * square" rule. */
int is_square_attacked(const Position *pos, int i, int j, Color by_side);

/* Whether side's king is attacked in the current position. */
int in_check(const Position *pos, Color side);

/* The complete set of legal moves in *pos. */
void generate_legal_moves(const Position *pos, MoveList *out);

/* The legal moves available to the single piece on (i, j) — the entry point a
 * move-highlight display uses, so it need not filter the full list itself. */
void generate_legal_moves_from(const Position *pos, int i, int j, MoveList *out);

#endif /* MOVEGEN_H */
