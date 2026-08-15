#include "core/board.h"

#include <string.h>

/* Function: init_board
 * The init_board function initializes the chess board with the standard starting positions for all pieces.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 *
 * The function performs the following steps:
 * 1. Defines a temporary board with the initial positions of all pieces.
 *    - The first two rows are filled with black pieces.
 *    - The last two rows are filled with white pieces.
 *    - The middle rows are empty.
 * 2. Copies the temporary board to the actual board using memcpy.
 */
void init_board(Piece_t board[8][8]) {
  Piece_t temp_board[8][8] = {
      {{BLACK, ROOK}, {BLACK, KNIGHT}, {BLACK, BISHOP}, {BLACK, QUEEN}, {BLACK, KING}, {BLACK, BISHOP}, {BLACK, KNIGHT}, {BLACK, ROOK}},
      {{BLACK, PAWN}, {BLACK, PAWN}, {BLACK, PAWN}, {BLACK, PAWN}, {BLACK, PAWN}, {BLACK, PAWN}, {BLACK, PAWN}, {BLACK, PAWN}},
      {{NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}},
      {{NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}},
      {{NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}},
      {{NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}, {NONE, FREE}},
      {{WHITE, PAWN}, {WHITE, PAWN}, {WHITE, PAWN}, {WHITE, PAWN}, {WHITE, PAWN}, {WHITE, PAWN}, {WHITE, PAWN}, {WHITE, PAWN}},
      {{WHITE, ROOK}, {WHITE, KNIGHT}, {WHITE, BISHOP}, {WHITE, QUEEN}, {WHITE, KING}, {WHITE, BISHOP}, {WHITE, KNIGHT}, {WHITE, ROOK}}};

  memcpy(board, temp_board, sizeof(temp_board));
}

/* Function: update_board
 * The update_board function updates the chess board by moving a piece from one position to another.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 * - prev_i: The row index of the piece's current position.
 * - prev_j: The column index of the piece's current position.
 * - next_i: The row index of the piece's target position.
 * - next_j: The column index of the piece's target position.
 *
 * A square is now just a color and a type, so the move is a struct assignment
 * and the vacated square is a struct literal.
 */
void update_board(Piece_t board[8][8], int prev_i, int prev_j, int next_i, int next_j) {
  board[next_i][next_j] = board[prev_i][prev_j];
  board[prev_i][prev_j] = (Piece_t){.color = NONE, .type = FREE};
}

/* Function: square_to_index
 * Converts a square name such as "e1" into board indices.
 *
 * Replaces find_piece_coordinates, which scanned all 64 squares comparing the
 * position string each one carried. The mapping is fixed by the board layout:
 * file a-h is column 0-7, and rank 8-1 is row 0-7.
 *
 * Returns 1 and writes *i and *j on success, 0 without touching them otherwise.
 */
int square_to_index(const char *pos, int *i, int *j) {
  if (pos == NULL || pos[0] < 'a' || pos[0] > 'h' || pos[1] < '1' || pos[1] > '8') {
    return 0;
  }

  *j = pos[0] - 'a';
  *i = '8' - pos[1];

  return 1;
}

/* Function: index_to_square
 * The inverse of square_to_index. Out-of-range indices produce "??".
 */
void index_to_square(int i, int j, char out[3]) {
  if (i < 0 || i > 7 || j < 0 || j > 7) {
    out[0] = '?';
    out[1] = '?';
  } else {
    out[0] = (char)('a' + j);
    out[1] = (char)('8' - i);
  }
  out[2] = '\0';
}
