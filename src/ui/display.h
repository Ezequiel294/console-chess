#ifndef DISPLAY_H
#define DISPLAY_H

#include "types.h"

#include <wchar.h>

/* All terminal output.
 *
 * A holding pen for the existing wprintf calls so that nothing else in the
 * program writes to the terminal. Replaced wholesale by the
 * terminal-ui-foundation change.
 */

/* The glyph for a piece. Empty squares and unknown colors render as a space.
 * Swapping this table is what an ASCII fallback amounts to. */
wchar_t piece_glyph(Piece_type_t type, Color color);

void print_board_white(Piece_t board[8][8]);
void print_board_black(Piece_t board[8][8]);
void print_history(History_node_t *p_history_head);
void print_captures(Captures_node_t *p_captures_head);

#endif /* DISPLAY_H */
