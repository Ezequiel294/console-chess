#ifndef RULES_H
#define RULES_H

#include "types.h"

/* Move legality.
 *
 * Pseudo-legal only: this layer knows how each piece moves and nothing about
 * check, castling, en passant, or promotion. Replaced wholesale by the
 * chess-rules-engine change.
 */

int is_valid_move(Piece_t board[8][8], int prev_i, int prev_j, int next_i, int next_j);

#endif /* RULES_H */
