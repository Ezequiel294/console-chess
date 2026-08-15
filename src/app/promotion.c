#include "app/promotion.h"

#include "ui/glyphs.h"
#include "ui/render.h"

#include <stdio.h>

/* Same idea as the board's own palette in game.c, kept separate rather than
 * shared: this overlay renders on its own filled background, not a square,
 * so it needs its own contrast rather than the board's. */
#define C_BOX_BG 236
#define C_BOX_FG 250
#define C_PIECE_WHITE 231
#define C_PIECE_BLACK 245

static const Piece_type_t CHOICES[4] = {QUEEN, ROOK, BISHOP, KNIGHT};
static const char *CHOICE_NAMES[4] = {"Queen", "Rook", "Bishop", "Knight"};

typedef struct {
  Color color;
  void (*on_choice)(void *ctx, Piece_type_t choice);
  void *ctx;
} Promotion_t;

static Promotion_t g_promotion;

static void promotion_render(void *ctx, Rect r) {
  Promotion_t *p = (Promotion_t *)ctx;
  int gw = glyphs_width();
  int w = 20;
  int h = 7;
  int x = (r.w - w) / 2;
  int y = (r.h - h) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  Rect box = rect_sub(r, x, y, w, h);

  draw_fill(box, ' ', COLOR_DEFAULT, C_BOX_BG, ATTR_NONE);
  draw_box(box, C_BOX_FG, C_BOX_BG, ATTR_NONE);

  Rect inner = rect_inset(box, 2, 1);
  draw_text(inner, 0, 0, "Promote to:", C_BOX_FG, C_BOX_BG, ATTR_BOLD);

  int fg = (p->color == BLACK) ? C_PIECE_BLACK : C_PIECE_WHITE;
  for (int k = 0; k < 4; k++) {
    draw_glyph(inner, 0, 2 + k, piece_glyph(CHOICES[k], p->color), gw, fg, C_BOX_BG, ATTR_NONE);
    char line[16];
    snprintf(line, sizeof(line), "%d) %s", k + 1, CHOICE_NAMES[k]);
    draw_text(inner, gw + 1, 2 + k, line, C_BOX_FG, C_BOX_BG, ATTR_NONE);
  }
}

static Cmd_t promotion_handle(void *ctx, const Event_t *ev) {
  Promotion_t *p = (Promotion_t *)ctx;
  if (ev->type != EV_KEY) {
    return CMD_STAY;
  }
  if (ev->key.name == KEY_ESCAPE) {
    return (Cmd_t){CMD_POP, NULL};
  }
  if (ev->key.name == KEY_CHAR) {
    for (int k = 0; k < 4; k++) {
      char digit = (char)('1' + k);
      char letter = "qrbn"[k];
      if ((char)ev->key.ch == digit || (char)ev->key.ch == letter ||
          (char)ev->key.ch == (char)(letter - 32)) {
        p->on_choice(p->ctx, CHOICES[k]);
        return (Cmd_t){CMD_POP, NULL};
      }
    }
  }
  return CMD_STAY;
}

Screen *promotion_screen(Color color, void (*on_choice)(void *ctx, Piece_type_t choice),
                          void *ctx) {
  static Screen screen;

  g_promotion.color = color;
  g_promotion.on_choice = on_choice;
  g_promotion.ctx = ctx;

  screen.on_enter = NULL;
  screen.on_exit = NULL;
  screen.handle = promotion_handle;
  screen.render = promotion_render;
  screen.ctx = &g_promotion;
  screen.opaque = 0; /* composites over the board beneath it */
  return &screen;
}
