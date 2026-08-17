#include "core/history.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Function: update_captures
 * The update_captures function adds a captured piece to the linked list of captures.
 *
 * Parameters:
 * - pp_captures_head: Double pointer to the head of the linked list of captured pieces.
 * - piece: The piece that has been captured.
 *
 * The function performs the following steps:
 * 1. Allocates memory for a new capture node. If memory allocation fails, prints an error message and exits.
 * 2. Initializes the new capture node with the captured piece and sets its next pointer to NULL.
 * 3. If the captures list is empty, sets the head of the list to the new node.
 * 4. If the captures list is not empty, traverses to the end of the list and adds the new node.
 */
void update_captures(Captures_node_t **pp_captures_head, Piece_t piece) {
  Captures_node_t *p_node = (Captures_node_t *)malloc(sizeof(Captures_node_t));
  if (p_node == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    exit(1);
  }

  p_node->piece = piece;
  p_node->p_next = NULL;

  if (*pp_captures_head == NULL) {
    *pp_captures_head = p_node;
  } else {
    Captures_node_t *p_current = *pp_captures_head;
    while (p_current->p_next != NULL) {
      p_current = p_current->p_next;
    }
    p_current->p_next = p_node;
  }
}

/* Function: update_history
 * The update_history function adds a new move to the linked list of move history.
 *
 * Parameters:
 * - pp_history_head: Double pointer to the head of the linked list of move history.
 * - prev_pos: The previous position of the piece as a string (e.g., "e2").
 * - next_pos: The next position of the piece as a string (e.g., "e4").
 *
 * The function performs the following steps:
 * 1. Allocates memory for a new history node. If memory allocation fails, prints an error message and exits.
 * 2. Initializes the new history node with the previous and next positions and sets its next pointer to NULL.
 * 3. If the history list is empty, sets the head of the list to the new node.
 * 4. If the history list is not empty, traverses to the end of the list and adds the new node.
 */
void update_history(History_node_t **pp_history_head, char prev_pos[3], char next_pos[3], Move move) {
  History_node_t *p_new_node = (History_node_t *)malloc(sizeof(History_node_t));
  if (p_new_node == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    exit(1);
  }

  strcpy(p_new_node->prev_pos, prev_pos);
  strcpy(p_new_node->next_pos, next_pos);
  p_new_node->move = move;
  p_new_node->p_next = NULL;

  history_push_node(pp_history_head, p_new_node);
}

History_node_t *history_pop_last(History_node_t **pp_history_head) {
  if (*pp_history_head == NULL) {
    return NULL;
  }
  if ((*pp_history_head)->p_next == NULL) {
    History_node_t *last = *pp_history_head;
    *pp_history_head = NULL;
    return last;
  }
  History_node_t *p_current = *pp_history_head;
  while (p_current->p_next->p_next != NULL) {
    p_current = p_current->p_next;
  }
  History_node_t *last = p_current->p_next;
  p_current->p_next = NULL;
  return last;
}

void history_push_node(History_node_t **pp_history_head, History_node_t *node) {
  node->p_next = NULL;
  if (*pp_history_head == NULL) {
    *pp_history_head = node;
  } else {
    History_node_t *p_current = *pp_history_head;
    while (p_current->p_next != NULL) {
      p_current = p_current->p_next;
    }
    p_current->p_next = node;
  }
}

Captures_node_t *captures_pop_last(Captures_node_t **pp_captures_head) {
  if (*pp_captures_head == NULL) {
    return NULL;
  }
  if ((*pp_captures_head)->p_next == NULL) {
    Captures_node_t *last = *pp_captures_head;
    *pp_captures_head = NULL;
    return last;
  }
  Captures_node_t *p_current = *pp_captures_head;
  while (p_current->p_next->p_next != NULL) {
    p_current = p_current->p_next;
  }
  Captures_node_t *last = p_current->p_next;
  p_current->p_next = NULL;
  return last;
}

/* Function: free_captures
 * The free_captures function frees the memory allocated for the linked list of captured pieces.
 *
 * Parameters:
 * - head: Pointer to the head of the linked list of captured pieces.
 *
 * The function performs the following steps:
 * 1. Iterates through the linked list.
 * 2. For each node, stores the next node in a temporary pointer.
 * 3. Frees the current node.
 * 4. Moves to the next node using the temporary pointer.
 * 5. Continues until all nodes are freed.
 */
void free_captures(Captures_node_t *head) {
  Captures_node_t *tmp;
  while (head != NULL) {
    tmp = head;
    head = head->p_next;
    free(tmp);
  }
}

/* Function: free_history
 * The free_history function frees the memory allocated for the linked list of move history.
 *
 * Parameters:
 * - head: Pointer to the head of the linked list of move history.
 *
 * The function performs the following steps:
 * 1. Iterates through the linked list.
 * 2. For each node, stores the next node in a temporary pointer.
 * 3. Frees the current node.
 * 4. Moves to the next node using the temporary pointer.
 * 5. Continues until all nodes are freed.
 */
void free_history(History_node_t *head) {
  History_node_t *tmp;
  while (head != NULL) {
    tmp = head;
    head = head->p_next;
    free(tmp);
  }
}

/* Function: push_hash
 * Appends a position hash to the end of the hash history, in the same
 * append-at-tail style as update_captures and update_history.
 */
void push_hash(Hash_node_t **pp_hash_head, uint64_t hash) {
  Hash_node_t *p_node = (Hash_node_t *)malloc(sizeof(Hash_node_t));
  if (p_node == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    exit(1);
  }

  p_node->hash = hash;
  p_node->p_next = NULL;

  if (*pp_hash_head == NULL) {
    *pp_hash_head = p_node;
  } else {
    Hash_node_t *p_current = *pp_hash_head;
    while (p_current->p_next != NULL) {
      p_current = p_current->p_next;
    }
    p_current->p_next = p_node;
  }
}

void free_hash_history(Hash_node_t *head) {
  Hash_node_t *tmp;
  while (head != NULL) {
    tmp = head;
    head = head->p_next;
    free(tmp);
  }
}

void hash_history_pop_last(Hash_node_t **pp_hash_head) {
  if (*pp_hash_head == NULL) {
    return;
  }
  if ((*pp_hash_head)->p_next == NULL) {
    free(*pp_hash_head);
    *pp_hash_head = NULL;
    return;
  }
  Hash_node_t *p_current = *pp_hash_head;
  while (p_current->p_next->p_next != NULL) {
    p_current = p_current->p_next;
  }
  free(p_current->p_next);
  p_current->p_next = NULL;
}

int hash_history_length(const Hash_node_t *head) {
  int n = 0;
  for (const Hash_node_t *p = head; p != NULL; p = p->p_next) {
    n++;
  }
  return n;
}

void hash_history_to_array(const Hash_node_t *head, uint64_t *out) {
  int i = 0;
  for (const Hash_node_t *p = head; p != NULL; p = p->p_next) {
    out[i++] = p->hash;
  }
}
