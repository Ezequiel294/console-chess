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
  /* The two clock rectangles, at the right edge beside the panel. Zero-width
   * for an untimed game, which is asked for by passing want_clock 0, and
   * also when the terminal is wide enough for the game but not for a clock
   * column beside a usable panel — in which case the game screen shows both
   * times compactly at the top of the panel instead. */
  Rect clock;
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

/* The width one clock rectangle takes, when there is room for one. */
#define LAYOUT_CLOCK_W 12

/* Divides bounds into regions. Returns 0 if bounds is below the minimum, in
 * which case out is untouched and the caller shows the too-small screen.
 *
 * want_clock asks for the clock column; pass 0 for an untimed game, whose
 * panel then takes the whole width beside the board exactly as it did before
 * a clock existed. The minimum size is the same either way — the clock is
 * taken from width that is already there, never from the board or the panel's
 * own minimum — so no terminal that plays today stops playing. */
int layout_compute(Rect bounds, int glyph_width, int want_clock, Layout *out);

#endif /* LAYOUT_H */
