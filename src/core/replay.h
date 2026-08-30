#ifndef REPLAY_H
#define REPLAY_H

#include "types.h"

/* Stepping a finished game's position back and forth through the moves it was
 * played with, built directly on the undo/redo mechanism in history.c and
 * unmake()/make() in movegen.c: nothing here is a second way to change a
 * position, only the two directions of the one way that already exists.
 *
 * Both functions move a single node between state->p_history_head and
 * state->p_redo_head and update state->position, state->p_hash_history_head,
 * and the captures lists to match — every field a stepped-to position needs
 * moves together, so the state is never left with the board on one move and
 * the hash or captures on another.
 */

/* Steps the position back to before its most recent move. Returns 1 if it
 * did, 0 if state->p_history_head is empty and there is nothing to undo. */
int replay_step_back(GameState *state);

/* Steps the position forward to after the next move on the redo list.
 * Returns 1 if it did, 0 if state->p_redo_head is empty and there is nothing
 * to redo. */
int replay_step_forward(GameState *state);

#endif /* REPLAY_H */
