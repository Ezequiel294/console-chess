#ifndef HISTORY_VIEW_H
#define HISTORY_VIEW_H

#include "app/app.h"
#include "types.h"

/* The scrollable, numbered SAN move list.
 *
 * state is borrowed and must not change while this screen is on top of the
 * stack — true by construction, since nothing else runs while it has input
 * focus.
 *
 * The list shown is always state->p_history_head ++ state->p_redo_head, the
 * whole game in played order; in live play p_redo_head is always empty, so
 * this is exactly the moves played so far. mark_current, when true, marks the
 * last move of p_history_head — the move the board is currently showing — and
 * opens the view scrolled to it; pass 0 for live play, where nothing should
 * be marked.
 */
Screen *history_view_screen(const GameState *state, int mark_current);

#endif /* HISTORY_VIEW_H */
