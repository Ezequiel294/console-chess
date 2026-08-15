#ifndef PROMOTION_H
#define PROMOTION_H

#include "app/app.h"
#include "types.h"

/* The overlay asking which piece a pawn promotes to.
 *
 * Composites over the board beneath it — the first real user of the screen
 * stack's opaque flag — and returns to it once a choice is made. It knows
 * nothing about Position or GameState: on_choice is called with the piece
 * selected (QUEEN, ROOK, BISHOP, or KNIGHT) just before the overlay pops
 * itself, and applying that choice to the game in progress is entirely the
 * caller's business. Pressing Escape pops without calling on_choice.
 */
Screen *promotion_screen(Color color, void (*on_choice)(void *ctx, Piece_type_t choice),
                          void *ctx);

#endif /* PROMOTION_H */
