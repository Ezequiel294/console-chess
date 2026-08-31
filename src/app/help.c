#include "app/help.h"

#include "ui/render.h"

#define C_BOX_BG 236
#define C_BOX_FG 250
#define C_HEADING 250
#define C_LABEL 246

static const char *LINES[] = {
    "Moving a piece",
    " Click a piece then its destination, or type the squares",
    " (e2 then e4, Enter after each), or press an arrow key for",
    " the board cursor and Enter on the square you want.",
    "",
    "Commands",
    " s  save          H  move history  x  resign",
    " o  offer draw    ?  this help     q  quit",
    " r  rename a save (on the Load Game screen)",
    "",
    "The clock",
    " SPACE is the clock press: your time runs until you press",
    " it, no overlay stops it, and running out ends the game.",
    "",
    "Mouse",
    " Click selects a piece or names its destination. The wheel",
    " scrolls the history screen and does nothing on the board.",
};
#define LINE_COUNT (int)(sizeof(LINES) / sizeof(LINES[0]))

/* A replay steps through a finished game rather than playing one: no piece is
 * ever picked up, so this describes stepping and flipping instead — a key
 * that does nothing here (s, x, o) is not listed at all. */
static const char *REPLAY_LINES[] = {
    "Stepping through the game",
    " u or ← steps back one move, r or → steps forward one —",
    " the only two ways the position on screen changes. No move",
    " can be made: clicking a piece or typing a square does",
    " nothing.",
    "",
    "Commands",
    " u ←  step back    h  move history  ?  this help",
    " r →  step forward f  flip board    q  quit",
    "",
    "Mouse",
    " The wheel scrolls the history screen and does nothing on",
    " the board — a replay is not clicked through.",
};
#define REPLAY_LINE_COUNT (int)(sizeof(REPLAY_LINES) / sizeof(REPLAY_LINES[0]))

static void render_lines(Rect r, const char *title, const char *const *lines, int line_count) {
  int w = 62;
  int h = line_count + 4;
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
  draw_text(inner, 0, 0, title, C_HEADING, C_BOX_BG, ATTR_BOLD);
  for (int i = 0; i < line_count && i + 2 < inner.h; i++) {
    uint8_t attr = (lines[i][0] != '\0' && lines[i][0] != ' ') ? ATTR_BOLD : ATTR_NONE;
    draw_text(inner, 0, i + 2, lines[i], C_BOX_FG, C_BOX_BG, attr);
  }

  app_draw_bottom_hint(r, "Esc close");
}

static void help_render(void *ctx, Rect r) {
  (void)ctx;
  render_lines(r, "How to Play", LINES, LINE_COUNT);
}

static void replay_help_render(void *ctx, Rect r) {
  (void)ctx;
  render_lines(r, "Replay", REPLAY_LINES, REPLAY_LINE_COUNT);
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
  static Screen screen = {NULL, NULL, help_handle, help_render, NULL, 0, NULL, NULL};
  return &screen;
}

Screen *replay_help_screen(void) {
  static Screen screen = {NULL, NULL, help_handle, replay_help_render, NULL, 0, NULL, NULL};
  return &screen;
}
