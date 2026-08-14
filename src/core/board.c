#include "core/board.h"

#include <string.h>

/* Function: print_board
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
      {{L'', BLACK, "a8", ROOK},
       {L'', BLACK, "b8", KNIGHT},
       {L'', BLACK, "c8", BISHOP},
       {L'', BLACK, "d8", QUEEN},
       {L'', BLACK, "e8", KING},
       {L'', BLACK, "f8", BISHOP},
       {L'', BLACK, "g8", KNIGHT},
       {L'', BLACK, "h8", ROOK}},
      {{L'', BLACK, "a7", PAWN},
       {L'', BLACK, "b7", PAWN},
       {L'', BLACK, "c7", PAWN},
       {L'', BLACK, "d7", PAWN},
       {L'', BLACK, "e7", PAWN},
       {L'', BLACK, "f7", PAWN},
       {L'', BLACK, "g7", PAWN},
       {L'', BLACK, "h7", PAWN}},
      {{L' ', NONE, "a6", FREE},
       {L' ', NONE, "b6", FREE},
       {L' ', NONE, "c6", FREE},
       {L' ', NONE, "d6", FREE},
       {L' ', NONE, "e6", FREE},
       {L' ', NONE, "f6", FREE},
       {L' ', NONE, "g6", FREE},
       {L' ', NONE, "h6", FREE}},
      {{L' ', NONE, "a5", FREE},
       {L' ', NONE, "b5", FREE},
       {L' ', NONE, "c5", FREE},
       {L' ', NONE, "d5", FREE},
       {L' ', NONE, "e5", FREE},
       {L' ', NONE, "f5", FREE},
       {L' ', NONE, "g5", FREE},
       {L' ', NONE, "h5", FREE}},
      {{L' ', NONE, "a4", FREE},
       {L' ', NONE, "b4", FREE},
       {L' ', NONE, "c4", FREE},
       {L' ', NONE, "d4", FREE},
       {L' ', NONE, "e4", FREE},
       {L' ', NONE, "f4", FREE},
       {L' ', NONE, "g4", FREE},
       {L' ', NONE, "h4", FREE}},
      {{L' ', NONE, "a3", FREE},
       {L' ', NONE, "b3", FREE},
       {L' ', NONE, "c3", FREE},
       {L' ', NONE, "d3", FREE},
       {L' ', NONE, "e3", FREE},
       {L' ', NONE, "f3", FREE},
       {L' ', NONE, "g3", FREE},
       {L' ', NONE, "h3", FREE}},
      {{L'󰡙', WHITE, "a2", PAWN},
       {L'󰡙', WHITE, "b2", PAWN},
       {L'󰡙', WHITE, "c2", PAWN},
       {L'󰡙', WHITE, "d2", PAWN},
       {L'󰡙', WHITE, "e2", PAWN},
       {L'󰡙', WHITE, "f2", PAWN},
       {L'󰡙', WHITE, "g2", PAWN},
       {L'󰡙', WHITE, "h2", PAWN}},
      {{L'󰡛', WHITE, "a1", ROOK},
       {L'󰡘', WHITE, "b1", KNIGHT},
       {L'󰡜', WHITE, "c1", BISHOP},
       {L'󰡚', WHITE, "d1", QUEEN},
       {L'󰡗', WHITE, "e1", KING},
       {L'󰡜', WHITE, "f1", BISHOP},
       {L'󰡘', WHITE, "g1", KNIGHT},
       {L'󰡛', WHITE, "h1", ROOK}}};

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
 * The function performs the following steps:
 * 1. Copies the piece from the previous position to the next position.
 * 2. Clears the previous position by setting it to an empty piece.
 */
void update_board(Piece_t board[8][8], int prev_i, int prev_j, int next_i, int next_j) {
  // Copy the piece from the previous position to the next position
  board[next_i][next_j].icon = board[prev_i][prev_j].icon;
  board[next_i][next_j].color = board[prev_i][prev_j].color;
  board[next_i][next_j].type = board[prev_i][prev_j].type;

  // Clear the previous position
  board[prev_i][prev_j].icon = L' ';
  board[prev_i][prev_j].color = NONE;
  board[prev_i][prev_j].type = FREE;
}

/* Function: find_piece_coordinates
 * The find_piece_coordinates function finds the coordinates of a piece on the chess board based on its position string.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 * - pos: The position of the piece as a string (e.g., "e2").
 * - i: Pointer to an integer where the row index of the piece will be stored.
 * - j: Pointer to an integer where the column index of the piece will be stored.
 *
 * The function performs the following steps:
 * 1. Iterates through each cell of the board.
 * 2. Compares the position string of each piece with the given position string.
 * 3. If a match is found, stores the coordinates in the provided pointers and returns 1.
 * 4. If no match is found after checking all cells, returns 0.
 */
int find_piece_coordinates(Piece_t board[8][8], char pos[3], int *i, int *j) {
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      if (board[x][y].position[0] == pos[0] && board[x][y].position[1] == pos[1]) {
        *i = x;
        *j = y;
        return 1;
      }
    }
  }
  return 0;
}
