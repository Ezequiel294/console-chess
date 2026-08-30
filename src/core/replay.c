#include "core/replay.h"

#include "core/history.h"
#include "core/movegen.h"

#include <stdlib.h>

int replay_step_back(GameState *state) {
  History_node_t *node = history_pop_last(&state->p_history_head);
  if (node == NULL) {
    return 0;
  }

  unmake(&state->position, node->move);
  hash_history_pop_last(&state->p_hash_history_head);

  if (node->move.captured != FREE) {
    /* unmake() restores side_to_move to the mover of the undone move, so the
     * capture came off that side's own list — see save.c's save_read, which
     * builds the same lists forward with the same rule. */
    Color mover = state->position.side_to_move;
    Captures_node_t **captures =
        (mover == WHITE) ? &state->p_captures_white_head : &state->p_captures_black_head;
    Captures_node_t *taken = captures_pop_last(captures);
    free(taken);
  }

  history_push_front(&state->p_redo_head, node);
  return 1;
}

int replay_step_forward(GameState *state) {
  History_node_t *node = history_pop_first(&state->p_redo_head);
  if (node == NULL) {
    return 0;
  }

  Color mover = state->position.side_to_move;

  make(&state->position, node->move);
  push_hash(&state->p_hash_history_head, state->position.hash);

  if (node->move.captured != FREE) {
    Color captured_color = (mover == WHITE) ? BLACK : WHITE;
    Captures_node_t **captures =
        (mover == WHITE) ? &state->p_captures_white_head : &state->p_captures_black_head;
    update_captures(captures, (Piece_t){.color = captured_color, .type = node->move.captured});
  }

  history_push_node(&state->p_history_head, node);
  return 1;
}
