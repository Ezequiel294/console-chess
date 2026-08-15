#ifndef POSITION_H
#define POSITION_H

#include "types.h"

/* The position: piece placement plus everything else legality depends on.
 *
 * board[0][0] is a8: row index 0 is rank 8 and column index 0 is file a,
 * matching both board.c and FEN's reading order.
 */

/* Fills *pos with the standard starting position: white to move, full
 * castling rights, no en passant target, both clocks at their initial
 * values, and the hash of that position. */
void position_init(Position *pos);

/* Recomputes pos->hash from the board and the rest of the position. Used
 * whenever a position is built other than by make() — position_init() and
 * fen_parse() — since make()/unmake() maintain the hash incrementally rather
 * than paying for a full recompute on every move. */
void position_compute_hash(Position *pos);

/* Bounds check: whether (i, j) names a square on the board. */
int square_on_board(int i, int j);

/* A square's own colour — light or dark, not the colour of any piece on it.
 * Used by the insufficient-material check: same-coloured bishops cannot force
 * mate. Returns 1 for a light square, 0 for a dark one. */
int square_is_light(int i, int j);

#endif /* POSITION_H */
