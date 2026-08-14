#include "core/history.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

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
    wprintf(L"Memory allocation failed.\n");
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
void update_history(History_node_t **pp_history_head, char prev_pos[3], char next_pos[3]) {
  History_node_t *p_new_node = (History_node_t *)malloc(sizeof(History_node_t));
  if (p_new_node == NULL) {
    wprintf(L"Memory allocation failed.\n");
    exit(1);
  }

  strcpy(p_new_node->prev_pos, prev_pos);
  strcpy(p_new_node->next_pos, next_pos);
  p_new_node->p_next = NULL;

  if (*pp_history_head == NULL) {
    *pp_history_head = p_new_node;
  } else {
    History_node_t *p_current = *pp_history_head;
    while (p_current->p_next != NULL) {
      p_current = p_current->p_next;
    }
    p_current->p_next = p_new_node;
  }
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
