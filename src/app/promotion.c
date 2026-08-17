#include "app/promotion.h"

#include "ui/glyphs.h"
#include "ui/render.h"

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

  int selected;
  int row_y[4]; /* where each choice was last drawn, for hit-testing a click */
} Promotion_t;

static Promotion_t g_promotion;

static void promotion_on_enter(void *ctx) { ((Promotion_t *)ctx)->selected = 0; }

static void promotion_render(void *ctx, Rect r) {
  Promotion_t *p = (Promotion_t *)ctx;
  int gw = glyphs_width();
  int w = 20;
  /* Two rows of border and padding, a title, a blank line, and one row per
   * choice — the last of which was silently clipped while this was 7. */
  int h = 8;
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
    uint8_t attr = (k == p->selected) ? ATTR_REVERSE : ATTR_NONE;
    draw_glyph(inner, 0, 2 + k, piece_glyph(CHOICES[k], p->color), gw, fg, C_BOX_BG, attr);
    draw_text(inner, gw + 1, 2 + k, CHOICE_NAMES[k], C_BOX_FG, C_BOX_BG, attr);
    p->row_y[k] = inner.y + 2 + k;
  }

  app_draw_bottom_hint(r, "↑/↓ + Enter, or click to select  ·  Esc cancel");
}

static Cmd_t promotion_handle(void *ctx, const Event_t *ev) {
  Promotion_t *p = (Promotion_t *)ctx;

  /* A click only moves the highlight, the same as an arrow key — Enter is the
   * one way to choose, since the piece this picks is permanent. */
  if (ev->type == EV_MOUSE && ev->mouse.kind == MOUSE_PRESS && ev->mouse.button == 0) {
    for (int k = 0; k < 4; k++) {
      if (ev->mouse.row == p->row_y[k]) {
        p->selected = k;
        break;
      }
    }
    return CMD_STAY;
  }
  if (ev->type != EV_KEY) {
    return CMD_STAY;
  }
  if (ev->key.name == KEY_ESCAPE) {
    return (Cmd_t){CMD_POP, NULL};
  }
  if (ev->key.name == KEY_UP) {
    p->selected = (p->selected + 3) % 4;
    return CMD_STAY;
  }
  if (ev->key.name == KEY_DOWN) {
    p->selected = (p->selected + 1) % 4;
    return CMD_STAY;
  }
  if (ev->key.name == KEY_ENTER) {
    p->on_choice(p->ctx, CHOICES[p->selected]);
    return (Cmd_t){CMD_POP, NULL};
  }
  return CMD_STAY;
}

Screen *promotion_screen(Color color, void (*on_choice)(void *ctx, Piece_type_t choice),
                          void *ctx) {
  static Screen screen;

  g_promotion.color = color;
  g_promotion.on_choice = on_choice;
  g_promotion.ctx = ctx;
  g_promotion.selected = 0;

  screen.on_enter = promotion_on_enter;
  screen.on_exit = NULL;
  screen.handle = promotion_handle;
  screen.render = promotion_render;
  screen.ctx = &g_promotion;
  screen.opaque = 0; /* composites over the board beneath it */
  return &screen;
}
