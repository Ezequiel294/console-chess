#include "ui/layout.h"

/* One cell of padding either side of the piece, so squares read as squares
 * rather than as a run of glyphs. */
#define SQUARE_PAD 1
#define PANEL_MIN_W 22
#define STATUS_H 3
#define TITLE_H 1

/* The grid's offset from the board block's own origin: a rank label and a
 * space to its left, a file label row above. board_block_w/h below add these
 * on both sides; grid_x/grid_y in Layout carry only the leading one, which is
 * the corner hit-testing and drawing both need. */
#define GRID_X 2
#define GRID_Y 1

/* The board's own grid, without labels: eight squares and the nine vertical
 * rules between and around them. */
static int board_grid_w(int glyph_width) {
  int square_w = glyph_width + 2 * SQUARE_PAD;
  return 8 * (square_w + 1) + 1;
}

static int board_grid_h(void) {
  return 8 * (1 + 1) + 1; /* eight one-row squares and their horizontal rules */
}

/* Plus a rank label and a space on each side. */
static int board_block_w(int glyph_width) { return board_grid_w(glyph_width) + 2 * GRID_X; }

/* Plus a file label row above and below. */
static int board_block_h(void) { return board_grid_h() + 2 * GRID_Y; }

int layout_min_cols(int glyph_width) {
  return board_block_w(glyph_width) + 1 + PANEL_MIN_W;
}

int layout_min_rows(int glyph_width) {
  (void)glyph_width; /* squares are one row tall at either glyph width */
  return TITLE_H + board_block_h() + STATUS_H;
}

int layout_compute(Rect bounds, int glyph_width, Layout *out) {
  if (bounds.w < layout_min_cols(glyph_width) || bounds.h < layout_min_rows(glyph_width)) {
    return 0;
  }

  int board_w = board_block_w(glyph_width);
  int board_h = board_block_h();

  out->square_w = glyph_width + 2 * SQUARE_PAD;
  out->square_h = 1;
  out->grid_x = GRID_X;
  out->grid_y = GRID_Y;

  out->title = rect_sub(bounds, 0, 0, bounds.w, TITLE_H);
  out->status = rect_sub(bounds, 0, bounds.h - STATUS_H, bounds.w, STATUS_H);

  /* The board keeps its natural size and the panel takes the rest, so growing
   * the terminal makes the panel more useful rather than stretching the board
   * into something that is no longer square. */
  int body_y = TITLE_H;
  int body_h = bounds.h - TITLE_H - STATUS_H;
  out->board = rect_sub(bounds, 0, body_y, board_w, board_h < body_h ? board_h : body_h);
  out->panel = rect_sub(bounds, board_w + 1, body_y, bounds.w - board_w - 1, body_h);

  return 1;
}
