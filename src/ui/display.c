#include "ui/display.h"

#include <wchar.h>

/* The one place a piece is turned into something a terminal can show.
 *
 * Indexed [color][type]. FREE is present in both rows so that an empty square
 * needs no special case at the call sites.
 */
static const wchar_t PIECE_GLYPHS[2][7] = {
    [WHITE] = {[PAWN] = L'󰡙', [ROOK] = L'󰡛', [KNIGHT] = L'󰡘', [BISHOP] = L'󰡜', [QUEEN] = L'󰡚', [KING] = L'󰡗', [FREE] = L' '},
    [BLACK] = {[PAWN] = L'', [ROOK] = L'', [KNIGHT] = L'', [BISHOP] = L'', [QUEEN] = L'', [KING] = L'', [FREE] = L' '},
};

wchar_t piece_glyph(Piece_type_t type, Color color) {
  // NONE is the color of an empty square, and the only other Color there is.
  if (color != WHITE && color != BLACK) {
    return L' ';
  }
  return PIECE_GLYPHS[color][type];
}

/* Function: print_board_white
 * The print_board_white function prints the chess board from the white player's perspective.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 *
 * The function performs the following steps:
 * 1. Prints the column labels (a to h).
 * 2. Prints the top border of the board.
 * 3. Iterates through each row of the board.
 * 4. For each row, prints the row number, the icons of the pieces in each column, and the row number again.
 * 5. Prints the border between rows.
 * 6. Prints the column labels (a to h) again at the bottom.
 */
void print_board_white(Piece_t board[8][8]) {
  wprintf(L"   a   b   c   d   e   f   g   h\n");
  wprintf(L" +---+---+---+---+---+---+---+---+\n");
  for (int i = 0; i < 8; i++) {
    wprintf(L"%d|", 8 - i);
    for (int j = 0; j < 8; j++) {
      wprintf(L" %lc |", piece_glyph(board[i][j].type, board[i][j].color));
    }
    wprintf(L" %d\n", 8 - i);
    wprintf(L" +---+---+---+---+---+---+---+---+\n");
  }
  wprintf(L"   a   b   c   d   e   f   g   h\n");
}

/* Function: print_board_black
 * The print_board_black function prints the chess board from the black player's perspective.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 *
 * The function performs the following steps:
 * 1. Prints the column labels (h to a).
 * 2. Prints the top border of the board.
 * 3. Iterates through each row of the board in reverse order (from 7 to 0).
 * 4. For each row, prints the row number, the icons of the pieces in each column in reverse order, and the row number again.
 * 5. Prints the border between rows.
 * 6. Prints the column labels (h to a) again at the bottom.
 */
void print_board_black(Piece_t board[8][8]) {
  wprintf(L"   h   g   f   e   d   c   b   a\n");
  wprintf(L" +---+---+---+---+---+---+---+---+\n");
  for (int i = 7; i >= 0; i--) {
    wprintf(L"%d|", 8 - i);
    for (int j = 7; j >= 0; j--) {
      wprintf(L" %lc |", piece_glyph(board[i][j].type, board[i][j].color));
    }
    wprintf(L" %d\n", 8 - i);
    wprintf(L" +---+---+---+---+---+---+---+---+\n");
  }
  wprintf(L"   h   g   f   e   d   c   b   a\n");
}

/* Function: print_history
 * The print_history function prints the move history of the chess game in a formatted table.
 *
 * Parameters:
 * - p_history_head: Pointer to the head of the linked list of move history.
 *
 * The function performs the following steps:
 * 1. Prints the header of the move history table.
 * 2. Iterates through the linked list of move history.
 * 3. For each node, prints the previous and next positions of the move.
 * 4. Continues until all moves in the history are printed.
 * 5. Prints the footer of the move history table.
 */
void print_history(History_node_t *p_history_head) {
  wprintf(L"\nMove History:\n");
  wprintf(L"+-----------------+\n");
  wprintf(L"|  From  |   To   |\n");
  wprintf(L"+-----------------+\n");
  History_node_t *p_current = p_history_head;
  while (p_current != NULL) {
    wprintf(L"|  %-5s |  %-5s |\n", p_current->prev_pos, p_current->next_pos);
    p_current = p_current->p_next;
  }
  wprintf(L"+-----------------+\n");
}

/* Function: print_captures
 * The print_captures function prints the icons of captured pieces from a linked list.
 *
 * Parameters:
 * - p_captures_head: Pointer to the head of the linked list of captured pieces.
 *
 * The function performs the following steps:
 * 1. Checks if the captures list is not empty.
 * 2. Iterates through the linked list of captured pieces.
 * 3. For each node, prints the icon of the captured piece.
 * 4. Continues until all captured pieces are printed.
 * 5. Prints a newline character at the end.
 */
void print_captures(Captures_node_t *p_captures_head) {
  if (p_captures_head != NULL) {
    Captures_node_t *p_current = p_captures_head;
    while (p_current != NULL) {
      wprintf(L"%lc ", piece_glyph(p_current->piece.type, p_current->piece.color));
      p_current = p_current->p_next;
    }
    wprintf(L"\n");
  }
}
