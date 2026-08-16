#ifndef INTERACTION_H
#define INTERACTION_H

#include "ui/layout.h"

/* Turns a point on screen into the square drawn there.
 *
 * Pure arithmetic over a Layout the caller already has — nothing here reads
 * the terminal, the position, or any other state, and nothing here is cached.
 * That is what keeps a click aligned with the frame it was aimed at: the
 * layout passed in must be the one the frame in question was actually drawn
 * with, never a freshly recomputed one, since a resize between the two would
 * make the two disagree about where the board is.
 */

typedef struct {
  int i, j;  /* board indices, meaningful only when valid is non-zero */
  int valid; /* 0 if the point named no square */
} Square_hit_t;

/* Resolves (col, row) — zero-based screen cells, the same space Event_t's
 * mouse fields use — to the square drawn there under lay and orientation.
 * flipped must match what draw_board was given for that frame: it is what
 * makes a click on a flipped board name what is drawn, not its mirror.
 *
 * A point on a grid rule, in the labels, or outside the board entirely
 * resolves to no square (valid = 0) rather than to the nearest one. */
Square_hit_t point_to_square(const Layout *lay, int col, int row, int flipped);

#endif /* INTERACTION_H */
