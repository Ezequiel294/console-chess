#ifndef HISTORY_H
#define HISTORY_H

#include "types.h"

/* The captured-pieces, move-history, and position-hash linked lists. */

void update_captures(Captures_node_t **pp_captures_head, Piece_t piece);
void update_history(History_node_t **pp_history_head, char prev_pos[3], char next_pos[3]);
void free_captures(Captures_node_t *head);
void free_history(History_node_t *head);

/* The hash history feeds outcome()'s threefold-repetition check: one entry
 * per position reached so far in the game, oldest first. */
void push_hash(Hash_node_t **pp_hash_head, uint64_t hash);
void free_hash_history(Hash_node_t *head);
int hash_history_length(const Hash_node_t *head);
/* Copies the list into out, oldest first. out must have room for at least
 * hash_history_length(head) entries. */
void hash_history_to_array(const Hash_node_t *head, uint64_t *out);

#endif /* HISTORY_H */
