#ifndef SAVE_H
#define SAVE_H

#include "types.h"

/* Persistence to game_save.bin.
 *
 * A raw dump of the in-memory structs, so the file format is tied to the
 * struct layout. Replaced by a text format in the app-shell-and-persistence
 * change.
 */
int save_game(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int moves);
int load_game(Piece_t board[8][8], Captures_node_t **p_captures_white_head, Captures_node_t **p_captures_black_head, History_node_t **p_history_head, int *moves);

#endif /* SAVE_H */
