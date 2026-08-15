#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

/* Frame composition.
 *
 * Nothing draws to the terminal directly. A frame is composed into an
 * off-screen grid of cells, compared against the grid currently on screen, and
 * only the differences are written — in one write, so a partly drawn frame is
 * never visible. An identical frame writes nothing at all.
 *
 * Every drawing primitive takes the Rect it is allowed to touch and clips to
 * it. Content that does not fit is cut at the edge; nothing wraps and nothing
 * escapes its rectangle, so a status line longer than the space available for
 * it cannot disturb anything else on screen.
 */

typedef struct {
  int x;
  int y;
  int w;
  int h;
} Rect;

/* Attributes. The cell reserves a byte; only what the board needs is
 * implemented. */
enum {
  ATTR_NONE = 0,
  ATTR_BOLD = 1 << 0,
  ATTR_REVERSE = 1 << 1,
  ATTR_DIM = 1 << 2
};

/* Colours are xterm-256 palette indices, or COLOR_DEFAULT for the terminal's
 * own foreground and background. */
#define COLOR_DEFAULT (-1)

typedef struct {
  uint32_t ch;  /* Unicode codepoint */
  int16_t fg;
  int16_t bg;
  uint8_t attr;
  /* Cells a glyph spans: 1 or 2 on the cell that carries it, 0 on the cell a
   * double-width glyph spills into. A continuation cell is never written to
   * the terminal; the wide glyph before it has already moved the cursor. */
  uint8_t width;
} Cell;

/* Allocates the grids for cols x rows. Returns 1 on success. */
int render_init(int cols, int rows);
void render_shutdown(void);

/* Discards both grids and reallocates. The next frame is a full repaint, which
 * is what a resize needs and is imperceptible. Returns 1 on success. */
int render_resize(int cols, int rows);

/* The whole screen, as a rect. */
Rect render_bounds(void);

/* Clears the frame being composed. */
void render_begin(void);

/* Compares the composed frame against what is on screen and writes the
 * difference. */
void render_flush(void);

/* Makes the next flush redraw everything. Exists so that a rendering bug can be
 * told apart from a state bug: if the forced repaint fixes the display, the
 * frame was composed correctly and the diff is at fault. */
void render_force_repaint(void);

/* --- Drawing primitives -------------------------------------------------
 *
 * x and y are relative to r's origin. Everything outside r is dropped.
 */

void draw_fill(Rect r, uint32_t ch, int fg, int bg, uint8_t attr);

/* Draws a codepoint occupying width cells. Dropped rather than half-drawn if
 * it would straddle the right edge. */
void draw_glyph(Rect r, int x, int y, uint32_t ch, int width, int fg, int bg, uint8_t attr);

/* Draws UTF-8 text, one cell per codepoint, truncated at the right edge of r.
 * Returns the number of cells drawn. */
int draw_text(Rect r, int x, int y, const char *utf8, int fg, int bg, uint8_t attr);

void draw_hline(Rect r, int x, int y, int len, uint32_t ch, int fg, int bg, uint8_t attr);
void draw_vline(Rect r, int x, int y, int len, uint32_t ch, int fg, int bg, uint8_t attr);

/* A single-line box around the edge of r. */
void draw_box(Rect r, int fg, int bg, uint8_t attr);

/* r narrowed to the region inside its border. */
Rect rect_inset(Rect r, int dx, int dy);

/* A sub-rectangle of r, in r's coordinates, clipped to r. */
Rect rect_sub(Rect r, int x, int y, int w, int h);

#endif /* RENDER_H */
