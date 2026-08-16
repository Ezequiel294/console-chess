#ifndef LAYOUT_H
#define LAYOUT_H

#include "ui/render.h"

/* Where things go, computed from the terminal's current size on every frame.
 *
 * Nothing here is stored between frames and nothing is hardcoded: the board's
 * width depends on how wide the terminal draws a piece, so the minimum size the
 * game needs depends on it too and has to be computed rather than assumed.
 */

typedef struct {
  Rect title;  /* one row: the game's name and the version */
  Rect board;  /* the grid plus its rank and file labels */
  Rect panel;  /* captures and move history, beside the board */
  Rect status; /* whose turn it is, the move being typed, the key hints */

  int square_w; /* interior width of one square, in cells */
  int square_h; /* interior height of one square, in cells */

  /* The grid's own top-left corner — the first rule line — as an offset from
   * board's origin, past the rank label and the file label row. draw_board
   * and point_to_square both read these rather than each assuming their own
   * offset, which is what keeps a click aligned with what was actually
   * drawn. */
  int grid_x;
  int grid_y;
} Layout;

/* The smallest terminal the game fits in, for a given piece glyph width. */
int layout_min_cols(int glyph_width);
int layout_min_rows(int glyph_width);

/* Divides bounds into regions. Returns 0 if bounds is below the minimum, in
 * which case out is untouched and the caller shows the too-small screen. */
int layout_compute(Rect bounds, int glyph_width, Layout *out);

#endif /* LAYOUT_H */
