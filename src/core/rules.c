#include "core/rules.h"

#include <stdlib.h>

// #define DEBUG

/* Function: is_valid_move
 * The is_valid_move function checks if a move in a chess game is valid based on the piece type and the rules of chess.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 * - prev_i: The row index of the piece's current position.
 * - prev_j: The column index of the piece's current position.
 * - next_i: The row index of the piece's target position.
 * - next_j: The column index of the piece's target position.
 *
 * The function performs the following steps:
 * 1. Retrieves the type of the piece being moved.
 * 2. Checks if the target position contains a piece of the same color, returning 0 if true.
 * 3. For each piece type (PAWN, ROOK, BISHOP, QUEEN, KING, KNIGHT), checks if the move follows the rules for that piece:
 *    - PAWN: Moves forward one or two squares (if it is its first move), or captures diagonally.
 *    - ROOK: Moves vertically or horizontally without jumping over other pieces.
 *    - BISHOP: Moves diagonally without jumping over other pieces.
 *    - QUEEN: Combines the movement rules of the rook and bishop.
 *    - KING: Moves one square in any direction.
 *    - KNIGHT: Moves in an L-shape.
 * 4. Returns 1 if the move is valid, otherwise returns 0.
 */
int is_valid_move(Piece_t board[8][8], int prev_i, int prev_j, int next_i, int next_j) {
  // Get the piece type being moved
  int piece_type = board[prev_i][prev_j].type;

  // Check if the player is not trying to capture their own piece
  if (board[next_i][next_j].color == board[prev_i][prev_j].color) {
    return 0;
  }

#ifdef DEBUG
  return 1;
#endif

  switch (piece_type) {
  case PAWN:
    switch (board[prev_i][prev_j].color) {
    case WHITE:
      // Moving one square forward
      if (next_i == prev_i - 1 && next_j == prev_j && board[next_i][next_j].type == FREE) {
        return 1;
      }
      // Moving two squares forward on the first move
      if (prev_i == 6 && next_i == prev_i - 2 && next_j == prev_j && board[next_i][next_j].type == FREE && board[prev_i - 1][next_j].type == FREE) {
        return 1;
      }
      // Capturing diagonally
      if (next_i == prev_i - 1 && (next_j == prev_j - 1 || next_j == prev_j + 1) && board[next_i][next_j].color == BLACK) {
        return 1;
      }
      break;
    case BLACK:
      // Moving one square forward
      if (next_i == prev_i + 1 && next_j == prev_j && board[next_i][next_j].type == FREE) {
        return 1;
      }
      // Moving two squares forward on the first move
      if (prev_i == 1 && next_i == prev_i + 2 && next_j == prev_j && board[next_i][next_j].type == FREE && board[prev_i + 1][next_j].type == FREE) {
        return 1;
      }
      // Capturing diagonally
      if (next_i == prev_i + 1 && (next_j == prev_j - 1 || next_j == prev_j + 1) && board[next_i][next_j].color == WHITE) {
        return 1;
      }
      break;
    case NONE:
      // An empty square has no moves. Unreachable, since the caller only ever
      // passes a square it has already checked is occupied, but naming the
      // case keeps -Wswitch useful if Color ever grows a member.
      break;
    }
    return 0;

  case ROOK:
    // Moving vertically
    if (next_j == prev_j) {
      int step = (next_i > prev_i) ? 1 : -1;
      for (int i = prev_i + step; i != next_i; i += step) {
        if (board[i][prev_j].type != FREE) {
          return 0;
        }
      }
      return 1;
    }
    // Moving horizontally
    if (next_i == prev_i) {
      int step = (next_j > prev_j) ? 1 : -1;
      for (int j = prev_j + step; j != next_j; j += step) {
        if (board[prev_i][j].type != FREE) {
          return 0;
        }
      }
      return 1;
    }
    return 0;

  case BISHOP:
    // Moving diagonally
    if (abs(next_i - prev_i) == abs(next_j - prev_j)) {
      int step_i = (next_i > prev_i) ? 1 : -1;
      int step_j = (next_j > prev_j) ? 1 : -1;
      int i = prev_i + step_i;
      int j = prev_j + step_j;
      while (i != next_i && j != next_j) {
        if (board[i][j].type != FREE) {
          return 0;
        }
        i += step_i;
        j += step_j;
      }
      return 1;
    }
    return 0;

  case QUEEN:
    // Combine rook and bishop logic
    // Moving vertically
    if (next_j == prev_j) {
      int step = (next_i > prev_i) ? 1 : -1;
      for (int i = prev_i + step; i != next_i; i += step) {
        if (board[i][prev_j].type != FREE) {
          return 0;
        }
      }
      return 1;
    }
    // Moving horizontally
    if (next_i == prev_i) {
      int step = (next_j > prev_j) ? 1 : -1;
      for (int j = prev_j + step; j != next_j; j += step) {
        if (board[prev_i][j].type != FREE) {
          return 0;
        }
      }
      return 1;
    }
    // Moving diagonally
    if (abs(next_i - prev_i) == abs(next_j - prev_j)) {
      int step_i = (next_i > prev_i) ? 1 : -1;
      int step_j = (next_j > prev_j) ? 1 : -1;
      int i = prev_i + step_i;
      int j = prev_j + step_j;
      while (i != next_i || j != next_j) {
        if (board[i][j].type != FREE) {
          return 0;
        }
        i += step_i;
        j += step_j;
      }
      return 1;
    }
    return 0;

  case KING:
    // Moving one square in any direction
    if (abs(next_i - prev_i) <= 1 && abs(next_j - prev_j) <= 1) {
      return 1;
    }
    return 0;

  case KNIGHT:
    // Moving in an L-shape
    if ((abs(next_i - prev_i) == 2 && abs(next_j - prev_j) == 1) || (abs(next_i - prev_i) == 1 && abs(next_j - prev_j) == 2)) {
      return 1;
    }
    return 0;

  default:
    return 0;
  }
}
