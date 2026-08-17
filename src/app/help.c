#include "app/help.h"

#include "ui/render.h"

#define C_BOX_BG 236
#define C_BOX_FG 250
#define C_HEADING 250
#define C_LABEL 246

static const char *LINES[] = {
    "Moving a piece",
    " Click a piece, then its destination — or type the square,",
    " e.g. e2 then e4, pressing Enter after each — or press an",
    " arrow key to bring up the board cursor, move it with the",
    " arrows, and press Enter on the square you want.",
    "",
    "Commands",
    " s  save          H  move history  x  resign",
    " o  offer draw    ?  this help     q  quit",
    "",
    "Mouse",
    " Click selects a piece or names its destination. The wheel",
    " scrolls the history screen and does nothing on the board.",
};
#define LINE_COUNT (int)(sizeof(LINES) / sizeof(LINES[0]))

static void help_render(void *ctx, Rect r) {
  (void)ctx;
  int w = 62;
  int h = LINE_COUNT + 4;
  if (w > r.w) w = r.w;
  if (h > r.h) h = r.h;
  int x = (r.w - w) / 2;
  int y = (r.h - h) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  Rect box = rect_sub(r, x, y, w, h);

  draw_fill(box, ' ', COLOR_DEFAULT, C_BOX_BG, ATTR_NONE);
  draw_box(box, C_BOX_FG, C_BOX_BG, ATTR_NONE);

  Rect inner = rect_inset(box, 2, 1);
  draw_text(inner, 0, 0, "How to Play", C_HEADING, C_BOX_BG, ATTR_BOLD);
  for (int i = 0; i < LINE_COUNT && i + 2 < inner.h; i++) {
    uint8_t attr = (LINES[i][0] != '\0' && LINES[i][0] != ' ') ? ATTR_BOLD : ATTR_NONE;
    draw_text(inner, 0, i + 2, LINES[i], C_BOX_FG, C_BOX_BG, attr);
  }

  app_draw_bottom_hint(r, "Esc close");
}

static Cmd_t help_handle(void *ctx, const Event_t *ev) {
  (void)ctx;
  if (ev->type == EV_KEY &&
      (ev->key.name == KEY_ESCAPE ||
       (ev->key.name == KEY_CHAR && (ev->key.ch == '?' || ev->key.ch == 'q')))) {
    return (Cmd_t){CMD_POP, NULL};
  }
  return CMD_STAY;
}

Screen *help_screen(void) {
  static Screen screen = {NULL, NULL, help_handle, help_render, NULL, 0};
  return &screen;
}
