#include "ui/render.h"

#include "ui/term.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The frame being composed, and the frame the terminal is currently showing. */
static Cell *g_compose = NULL;
static Cell *g_screen = NULL;
static int g_cols = 0;
static int g_rows = 0;
static int g_full_repaint = 1;

/* One growable buffer, reused every frame, so a flush is a single write and the
 * terminal never shows a half-drawn frame. */
static char *g_out = NULL;
static size_t g_out_len = 0;
static size_t g_out_cap = 0;

static const Cell BLANK = {(uint32_t)' ', COLOR_DEFAULT, COLOR_DEFAULT, ATTR_NONE, 1};

/* A cell that can equal nothing, used to mark the screen grid unknown so that
 * every cell counts as changed. */
static const Cell UNKNOWN = {0xFFFFFFFFu, -2, -2, 0xFF, 0xFF};

/* --- Output buffer ------------------------------------------------------ */

static void out_reserve(size_t extra) {
  if (g_out_len + extra <= g_out_cap) {
    return;
  }
  size_t cap = g_out_cap ? g_out_cap : 4096;
  while (cap < g_out_len + extra) {
    cap *= 2;
  }
  char *grown = realloc(g_out, cap);
  if (grown == NULL) {
    return; /* the frame is dropped rather than the program; the next one retries */
  }
  g_out = grown;
  g_out_cap = cap;
}

static void out_bytes(const char *s, size_t n) {
  out_reserve(n);
  if (g_out_len + n > g_out_cap) {
    return;
  }
  memcpy(g_out + g_out_len, s, n);
  g_out_len += n;
}

static void out_str(const char *s) { out_bytes(s, strlen(s)); }

static void out_fmt1(const char *fmt, int a) {
  char tmp[48];
  int n = snprintf(tmp, sizeof(tmp), fmt, a);
  if (n > 0) {
    out_bytes(tmp, (size_t)n);
  }
}

static void out_fmt2(const char *fmt, int a, int b) {
  char tmp[48];
  int n = snprintf(tmp, sizeof(tmp), fmt, a, b);
  if (n > 0) {
    out_bytes(tmp, (size_t)n);
  }
}

static void out_utf8(uint32_t cp) {
  char tmp[4];
  size_t n;
  if (cp < 0x80u) {
    tmp[0] = (char)cp;
    n = 1;
  } else if (cp < 0x800u) {
    tmp[0] = (char)(0xC0u | (cp >> 6));
    tmp[1] = (char)(0x80u | (cp & 0x3Fu));
    n = 2;
  } else if (cp < 0x10000u) {
    tmp[0] = (char)(0xE0u | (cp >> 12));
    tmp[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
    tmp[2] = (char)(0x80u | (cp & 0x3Fu));
    n = 3;
  } else {
    tmp[0] = (char)(0xF0u | (cp >> 18));
    tmp[1] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
    tmp[2] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
    tmp[3] = (char)(0x80u | (cp & 0x3Fu));
    n = 4;
  }
  out_bytes(tmp, n);
}

/* --- Grids -------------------------------------------------------------- */

static void fill_grid(Cell *grid, Cell value, int count) {
  for (int i = 0; i < count; i++) {
    grid[i] = value;
  }
}

int render_init(int cols, int rows) {
  g_compose = NULL;
  g_screen = NULL;
  g_cols = 0;
  g_rows = 0;
  return render_resize(cols, rows);
}

void render_shutdown(void) {
  free(g_compose);
  free(g_screen);
  free(g_out);
  g_compose = NULL;
  g_screen = NULL;
  g_out = NULL;
  g_out_cap = 0;
  g_out_len = 0;
  g_cols = 0;
  g_rows = 0;
}

int render_resize(int cols, int rows) {
  if (cols < 1 || rows < 1) {
    return 0;
  }
  size_t count = (size_t)cols * (size_t)rows;
  Cell *compose = realloc(g_compose, count * sizeof(Cell));
  Cell *screen = realloc(g_screen, count * sizeof(Cell));
  if (compose == NULL || screen == NULL) {
    free(compose == NULL ? g_compose : compose);
    free(screen == NULL ? g_screen : screen);
    g_compose = NULL;
    g_screen = NULL;
    g_cols = 0;
    g_rows = 0;
    return 0;
  }
  g_compose = compose;
  g_screen = screen;
  g_cols = cols;
  g_rows = rows;

  fill_grid(g_compose, BLANK, cols * rows);
  fill_grid(g_screen, UNKNOWN, cols * rows);
  g_full_repaint = 1;
  return 1;
}

Rect render_bounds(void) {
  Rect r = {0, 0, g_cols, g_rows};
  return r;
}

void render_begin(void) {
  if (g_compose != NULL) {
    fill_grid(g_compose, BLANK, g_cols * g_rows);
  }
}

void render_force_repaint(void) { g_full_repaint = 1; }

/* --- Flush -------------------------------------------------------------- */

static int cell_equal(const Cell *a, const Cell *b) {
  return a->ch == b->ch && a->fg == b->fg && a->bg == b->bg &&
         a->attr == b->attr && a->width == b->width;
}

/* Emits the SGR needed to move from the pen's current colours to the cell's.
 * Tracked across the whole frame, so a run of same-coloured cells costs one
 * escape sequence rather than one per cell. */
static void emit_style(const Cell *c, int16_t *fg, int16_t *bg, uint8_t *attr) {
  if (c->fg == *fg && c->bg == *bg && c->attr == *attr) {
    return;
  }
  /* Attributes have no individual "off" that is portable enough to rely on, so
   * a change resets and rebuilds. Colour-only changes take the cheap path. */
  if (c->attr != *attr) {
    out_str("\033[0m");
    *fg = COLOR_DEFAULT;
    *bg = COLOR_DEFAULT;
    *attr = ATTR_NONE;
    if (c->attr & ATTR_BOLD) {
      out_str("\033[1m");
    }
    if (c->attr & ATTR_DIM) {
      out_str("\033[2m");
    }
    if (c->attr & ATTR_REVERSE) {
      out_str("\033[7m");
    }
    *attr = c->attr;
  }
  if (c->fg != *fg) {
    if (c->fg == COLOR_DEFAULT) {
      out_str("\033[39m");
    } else {
      out_fmt1("\033[38;5;%dm", c->fg);
    }
    *fg = c->fg;
  }
  if (c->bg != *bg) {
    if (c->bg == COLOR_DEFAULT) {
      out_str("\033[49m");
    } else {
      out_fmt1("\033[48;5;%dm", c->bg);
    }
    *bg = c->bg;
  }
}

void render_flush(void) {
  if (g_compose == NULL || g_screen == NULL) {
    return;
  }

  g_out_len = 0;

  /* The pen: where the cursor is and what colour it is drawing in. Row -1 means
   * "not positioned", which forces the first run to emit a cursor move. */
  int pen_row = -1;
  int pen_col = -1;
  int16_t fg = COLOR_DEFAULT;
  int16_t bg = COLOR_DEFAULT;
  uint8_t attr = ATTR_NONE;
  out_str("\033[0m");

  for (int y = 0; y < g_rows; y++) {
    Cell *crow = g_compose + (size_t)y * (size_t)g_cols;
    Cell *srow = g_screen + (size_t)y * (size_t)g_cols;

    int x = 0;
    while (x < g_cols) {
      if (!g_full_repaint && cell_equal(&crow[x], &srow[x])) {
        x++;
        continue;
      }

      /* A changed continuation cell means its lead cell must be redrawn too:
       * the terminal draws a wide glyph from its lead, and re-emitting only the
       * tail would leave half a piece on screen. */
      int start = x;
      while (start > 0 && crow[start].width == 0) {
        start--;
      }

      int end = start;
      int gap = 0;
      while (end < g_cols) {
        int changed = g_full_repaint || !cell_equal(&crow[end], &srow[end]);
        if (changed) {
          gap = 0;
          end++;
          /* A wide glyph carries its continuation cell with it. */
          if (crow[end - 1].width == 2 && end < g_cols) {
            end++;
          }
          continue;
        }
        /* A short unchanged stretch is cheaper to redraw than to jump over:
         * a cursor move costs about six bytes. */
        if (gap < 6) {
          gap++;
          end++;
          continue;
        }
        end -= gap;
        break;
      }
      if (end > g_cols) {
        end = g_cols;
      }
      /* Trim any trailing unchanged cells the gap tolerance let through. */
      while (end > start && !g_full_repaint && cell_equal(&crow[end - 1], &srow[end - 1])) {
        end--;
      }

      if (pen_row != y || pen_col != start) {
        out_fmt2("\033[%d;%dH", y + 1, start + 1);
        pen_row = y;
        pen_col = start;
      }

      for (int i = start; i < end; i++) {
        if (crow[i].width == 0) {
          continue; /* covered by the wide glyph before it */
        }
        emit_style(&crow[i], &fg, &bg, &attr);
        out_utf8(crow[i].ch);
        pen_col += crow[i].width;
      }

      x = end > start ? end : start + 1;
    }

    memcpy(srow, crow, (size_t)g_cols * sizeof(Cell));
  }

  g_full_repaint = 0;

  /* An unchanged frame gets this far having emitted only the leading reset,
   * which is not worth a write. Nothing goes to the terminal at all. */
  if (g_out_len <= 4) {
    g_out_len = 0;
    return;
  }

  out_str("\033[0m");
  term_write(g_out, g_out_len);
  g_out_len = 0;
}

/* --- Primitives --------------------------------------------------------- */

/* Every primitive funnels through here, which is what makes clipping a property
 * of the module rather than of each caller. */
static void put(Rect r, int x, int y, uint32_t ch, int width, int fg, int bg, uint8_t attr) {
  if (g_compose == NULL) {
    return;
  }
  if (x < 0 || y < 0 || y >= r.h || x >= r.w) {
    return;
  }
  if (width == 2 && x + 1 >= r.w) {
    return; /* would straddle the edge: drop it rather than draw half a piece */
  }

  int ax = r.x + x;
  int ay = r.y + y;
  if (ax < 0 || ay < 0 || ax >= g_cols || ay >= g_rows) {
    return;
  }
  if (width == 2 && ax + 1 >= g_cols) {
    return;
  }

  Cell *row = g_compose + (size_t)ay * (size_t)g_cols;

  /* Overwriting one half of a wide glyph already in the grid would leave the
   * other half orphaned, so the orphan is blanked. */
  if (row[ax].width == 0 && ax > 0) {
    row[ax - 1] = BLANK;
  }
  if (row[ax].width == 2 && ax + 1 < g_cols) {
    row[ax + 1] = BLANK;
  }

  row[ax].ch = ch;
  row[ax].fg = (int16_t)fg;
  row[ax].bg = (int16_t)bg;
  row[ax].attr = attr;
  row[ax].width = (uint8_t)width;

  if (width == 2) {
    if (row[ax + 1].width == 2 && ax + 2 < g_cols) {
      row[ax + 2] = BLANK;
    }
    row[ax + 1].ch = 0;
    row[ax + 1].fg = (int16_t)fg;
    row[ax + 1].bg = (int16_t)bg;
    row[ax + 1].attr = attr;
    row[ax + 1].width = 0;
  }
}

void draw_fill(Rect r, uint32_t ch, int fg, int bg, uint8_t attr) {
  for (int y = 0; y < r.h; y++) {
    for (int x = 0; x < r.w; x++) {
      put(r, x, y, ch, 1, fg, bg, attr);
    }
  }
}

void draw_glyph(Rect r, int x, int y, uint32_t ch, int width, int fg, int bg, uint8_t attr) {
  put(r, x, y, ch, width == 2 ? 2 : 1, fg, bg, attr);
}

/* Decodes one UTF-8 codepoint. Returns the bytes consumed, or 0 at the end of
 * the string. Malformed input consumes one byte and yields U+FFFD rather than
 * stalling. */
static int utf8_next(const char *s, uint32_t *out) {
  const unsigned char *p = (const unsigned char *)s;
  if (p[0] == 0) {
    return 0;
  }
  if (p[0] < 0x80u) {
    *out = p[0];
    return 1;
  }
  int len;
  uint32_t cp;
  if ((p[0] & 0xE0u) == 0xC0u) {
    len = 2;
    cp = p[0] & 0x1Fu;
  } else if ((p[0] & 0xF0u) == 0xE0u) {
    len = 3;
    cp = p[0] & 0x0Fu;
  } else if ((p[0] & 0xF8u) == 0xF0u) {
    len = 4;
    cp = p[0] & 0x07u;
  } else {
    *out = 0xFFFDu;
    return 1;
  }
  for (int i = 1; i < len; i++) {
    if ((p[i] & 0xC0u) != 0x80u) {
      *out = 0xFFFDu;
      return 1;
    }
    cp = (cp << 6) | (p[i] & 0x3Fu);
  }
  *out = cp;
  return len;
}

int draw_text(Rect r, int x, int y, const char *utf8, int fg, int bg, uint8_t attr) {
  if (utf8 == NULL) {
    return 0;
  }
  int drawn = 0;
  const char *p = utf8;
  uint32_t cp;
  int n;
  while ((n = utf8_next(p, &cp)) > 0) {
    if (x + drawn >= r.w) {
      break; /* truncated at the edge of the region; never wrapped */
    }
    put(r, x + drawn, y, cp, 1, fg, bg, attr);
    drawn++;
    p += n;
  }
  return drawn;
}

void draw_hline(Rect r, int x, int y, int len, uint32_t ch, int fg, int bg, uint8_t attr) {
  for (int i = 0; i < len; i++) {
    put(r, x + i, y, ch, 1, fg, bg, attr);
  }
}

void draw_vline(Rect r, int x, int y, int len, uint32_t ch, int fg, int bg, uint8_t attr) {
  for (int i = 0; i < len; i++) {
    put(r, x, y + i, ch, 1, fg, bg, attr);
  }
}

void draw_box(Rect r, int fg, int bg, uint8_t attr) {
  if (r.w < 2 || r.h < 2) {
    return;
  }
  draw_hline(r, 1, 0, r.w - 2, 0x2500u, fg, bg, attr);
  draw_hline(r, 1, r.h - 1, r.w - 2, 0x2500u, fg, bg, attr);
  draw_vline(r, 0, 1, r.h - 2, 0x2502u, fg, bg, attr);
  draw_vline(r, r.w - 1, 1, r.h - 2, 0x2502u, fg, bg, attr);
  put(r, 0, 0, 0x250Cu, 1, fg, bg, attr);
  put(r, r.w - 1, 0, 0x2510u, 1, fg, bg, attr);
  put(r, 0, r.h - 1, 0x2514u, 1, fg, bg, attr);
  put(r, r.w - 1, r.h - 1, 0x2518u, 1, fg, bg, attr);
}

Rect rect_inset(Rect r, int dx, int dy) {
  Rect out;
  out.x = r.x + dx;
  out.y = r.y + dy;
  out.w = r.w - 2 * dx;
  out.h = r.h - 2 * dy;
  if (out.w < 0) {
    out.w = 0;
  }
  if (out.h < 0) {
    out.h = 0;
  }
  return out;
}

Rect rect_sub(Rect r, int x, int y, int w, int h) {
  Rect out;
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > r.w) {
    w = r.w - x;
  }
  if (y + h > r.h) {
    h = r.h - y;
  }
  out.x = r.x + x;
  out.y = r.y + y;
  out.w = w < 0 ? 0 : w;
  out.h = h < 0 ? 0 : h;
  return out;
}
