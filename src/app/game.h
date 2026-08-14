#ifndef GAME_H
#define GAME_H

#include "types.h"

/* Turn flow: whose move it is, asking for it, applying it, deciding when the
 * game is over. Becomes the Game screen in the terminal-ui-foundation change.
 */
void game_loop(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int *moves);
void get_move(Piece_t board[8][8], Captures_node_t **pp_capture_color_head, History_node_t **pp_history_head, int *captured_king, int *moves);

#endif /* GAME_H */
