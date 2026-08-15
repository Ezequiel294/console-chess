#include "app/toosmall.h"

#include "ui/glyphs.h"
#include "ui/layout.h"

#include <stdio.h>

static void toosmall_render(void *ctx, Rect r) {
  (void)ctx;

  int width = glyphs_width();
  char line[64];

  draw_fill(r, ' ', COLOR_DEFAULT, COLOR_DEFAULT, ATTR_NONE);

  int y = r.h / 2 - 2;
  if (y < 0) {
    y = 0;
  }

  /* Centred where there is room, flush left where there is not — the message
   * about the terminal being too small must survive a terminal that is very
   * small indeed. */
  const char *heading = "Terminal too small";
  int x = (r.w - (int)sizeof("Terminal too small") + 1) / 2;
  if (x < 0) {
    x = 0;
  }
  draw_text(r, x, y, heading, COLOR_DEFAULT, COLOR_DEFAULT, ATTR_BOLD);

  snprintf(line, sizeof(line), "need %d x %d", layout_min_cols(width), layout_min_rows(width));
  draw_text(r, x, y + 2, line, COLOR_DEFAULT, COLOR_DEFAULT, ATTR_NONE);

  snprintf(line, sizeof(line), "have %d x %d", r.w, r.h);
  draw_text(r, x, y + 3, line, COLOR_DEFAULT, COLOR_DEFAULT, ATTR_NONE);

  draw_text(r, x, y + 5, "resize, or q to quit", COLOR_DEFAULT, COLOR_DEFAULT, ATTR_DIM);
}

static Cmd_t toosmall_handle(void *ctx, const Event_t *ev) {
  (void)ctx;
  if (ev->type == EV_KEY && ev->key.name == KEY_CHAR &&
      (ev->key.ch == 'q' || ev->key.ch == 'Q')) {
    Cmd_t quit = {CMD_QUIT, NULL};
    return quit;
  }
  return CMD_STAY;
}

Screen *toosmall_screen(void) {
  static Screen screen = {NULL, NULL, toosmall_handle, toosmall_render, NULL, 1};
  return &screen;
}
