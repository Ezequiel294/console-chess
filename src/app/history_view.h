#ifndef HISTORY_VIEW_H
#define HISTORY_VIEW_H

#include "app/app.h"
#include "types.h"

/* The scrollable, numbered SAN move list.
 *
 * state is borrowed and must not change while this screen is on top of the
 * stack — true by construction, since nothing else runs while it has input
 * focus. Undone moves are simply absent from p_history_head (see move-undo),
 * so this screen needs no special case for them: it shows exactly the list it
 * is given.
 */
Screen *history_view_screen(const GameState *state);

#endif /* HISTORY_VIEW_H */
