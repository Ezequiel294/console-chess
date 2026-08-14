#ifndef HISTORY_H
#define HISTORY_H

#include "types.h"

/* The captured-pieces and move-history linked lists. */

void update_captures(Captures_node_t **pp_captures_head, Piece_t piece);
void update_history(History_node_t **pp_history_head, char prev_pos[3], char next_pos[3]);
void free_captures(Captures_node_t *head);
void free_history(History_node_t *head);

#endif /* HISTORY_H */
