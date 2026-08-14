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
int find_piece_coordinates(Piece_t board[8][8], char pos[3], int *i, int *j);

#endif /* BOARD_H */
