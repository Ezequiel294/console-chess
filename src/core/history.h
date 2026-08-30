#ifndef HISTORY_H
#define HISTORY_H

#include "types.h"

/* The captured-pieces, move-history, and position-hash linked lists. */

void update_captures(Captures_node_t **pp_captures_head, Piece_t piece);
void update_history(History_node_t **pp_history_head, char prev_pos[3], char next_pos[3], Move move);
void free_captures(Captures_node_t *head);
void free_history(History_node_t *head);

/* Detaches and returns the last node (NULL if empty), for undo popping the
 * most recent move or capture. The returned node's p_next is NULL. */
History_node_t *history_pop_last(History_node_t **pp_history_head);
Captures_node_t *captures_pop_last(Captures_node_t **pp_captures_head);

/* Detaches and returns the first node (NULL if empty), for redo stepping
 * forward through the earliest move undone so far — the head of a redo list
 * built by history_push_front. The returned node's p_next is NULL. */
History_node_t *history_pop_first(History_node_t **pp_history_head);

/* Prepends an already-built node (its p_next is overwritten) to the head, for
 * undo moving a node from the history list onto the redo list: each
 * successive undo prepends the move it removes, so the redo list's head stays
 * the chronologically earliest undone move and history ++ redo stays the
 * whole game in played order. */
void history_push_front(History_node_t **pp_history_head, History_node_t *node);

/* Appends an already-built node (its p_next is overwritten) to the tail, for
 * redo re-inserting a node popped earlier by history_pop_last. */
void history_push_node(History_node_t **pp_history_head, History_node_t *node);

/* Removes and frees the last hash, undoing the push_hash() that accompanied
 * the move being undone. */
void hash_history_pop_last(Hash_node_t **pp_hash_head);

/* The hash history feeds outcome()'s threefold-repetition check: one entry
 * per position reached so far in the game, oldest first. */
void push_hash(Hash_node_t **pp_hash_head, uint64_t hash);
void free_hash_history(Hash_node_t *head);
int hash_history_length(const Hash_node_t *head);
/* Copies the list into out, oldest first. out must have room for at least
 * hash_history_length(head) entries. */
void hash_history_to_array(const Hash_node_t *head, uint64_t *out);

#endif /* HISTORY_H */
