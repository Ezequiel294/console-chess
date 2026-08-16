#include "ui/interaction.h"

static const Square_hit_t NO_HIT = {0, 0, 0};

Square_hit_t point_to_square(const Layout *lay, int col, int row, int flipped) {
  /* The grid's own top-left rule, in the same absolute screen space as col
   * and row: board's origin plus the offset past the labels. */
  int gx0 = lay->board.x + lay->grid_x;
  int gy0 = lay->board.y + lay->grid_y;

  int rel_x = col - gx0;
  int rel_y = row - gy0;
  if (rel_x < 0 || rel_y < 0) {
    return NO_HIT;
  }

  /* Columns: a vertical rule, then square_w interior cells, repeating. Column
   * rel_x lands on the rule itself when it divides evenly — between squares,
   * not inside one. */
  int step_x = lay->square_w + 1;
  int scr_col = rel_x / step_x;
  int rem_x = rel_x % step_x;
  if (rem_x == 0 || scr_col > 7) {
    return NO_HIT;
  }

  /* Rows: a horizontal rule, then one interior row, repeating — square_h is
   * always 1, so the period is fixed at 2 regardless of glyph width. */
  if (rel_y % 2 == 0) {
    return NO_HIT; /* on a horizontal rule */
  }
  int scr_row = (rel_y - 1) / 2;
  if (scr_row > 7) {
    return NO_HIT;
  }

  Square_hit_t hit;
  hit.i = flipped ? 7 - scr_row : scr_row;
  hit.j = flipped ? 7 - scr_col : scr_col;
  hit.valid = 1;
  return hit;
}
