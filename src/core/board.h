#ifndef BOARD_H
#define BOARD_H

#include "types.h"

/* Board representation and the squares themselves.
 *
 * board[0][0] is a8: row index 0 is rank 8 and column index 0 is file a, which
 * matches both the printing order and FEN's reading order.
 */

void init_board(Piece_t board[8][8]);
void update_board(Piece_t board[8][8], int prev_i, int prev_j, int next_i, int next_j);

/* Square name to indices, e.g. "e1" -> i=7, j=4. Returns 0 and leaves the
 * outputs untouched if pos is not a square name. */
int square_to_index(const char *pos, int *i, int *j);

/* The inverse. out must have room for three chars. */
void index_to_square(int i, int j, char out[3]);

#endif /* BOARD_H */
