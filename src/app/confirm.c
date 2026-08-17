#include "app/confirm.h"

#include "ui/render.h"

#include <stdio.h>
#include <string.h>

#define C_BOX_BG 236
#define C_BOX_FG 250

/* The answer a confirmation opens on. */
#define CONFIRM_NO 1

typedef struct {
  char message[128];
  Cmd_t (*on_yes)(void *ctx);
  Cmd_t (*on_no)(void *ctx);
  void *ctx;

  /* 0 = yes, 1 = no. Starts on "no": every confirmation this screen is used
   * for ends the game one way or another, and Enter is now the only thing
   * that acts on a highlight (a click merely moves it), so the highlight is
   * the last remaining way to end a game by accident. Reaching "yes" is one
   * arrow key; landing on it by default is a mistake that cannot be undone. */
  int selected;
  /* Where each answer's text was last drawn, for hit-testing a click; both
   * sit on one row, so the click test is by column range, not by row. */
  int row_y;
  int yes_x0, yes_x1;
  int no_x0, no_x1;
} Confirm_t;

static Confirm_t g_confirm;

static void confirm_on_enter(void *ctx) { ((Confirm_t *)ctx)->selected = CONFIRM_NO; }

static void confirm_render(void *ctx, Rect r) {
  Confirm_t *c = (Confirm_t *)ctx;
  int w = (int)strlen(c->message) + 4;
  if (w < 24) {
    w = 24;
  }
  if (w > r.w) {
    w = r.w;
  }
  int h = 5;
  int x = (r.w - w) / 2;
  int y = (r.h - h) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  Rect box = rect_sub(r, x, y, w, h);

  draw_fill(box, ' ', COLOR_DEFAULT, C_BOX_BG, ATTR_NONE);
  draw_box(box, C_BOX_FG, C_BOX_BG, ATTR_NONE);

  Rect inner = rect_inset(box, 2, 1);
  draw_text(inner, 0, 0, c->message, C_BOX_FG, C_BOX_BG, ATTR_NONE);

  uint8_t yes_attr = (c->selected == 0) ? ATTR_REVERSE : ATTR_NONE;
  uint8_t no_attr = (c->selected == 1) ? ATTR_REVERSE : ATTR_NONE;
  int cx = 0;
  c->yes_x0 = inner.x + cx;
  cx += draw_text(inner, cx, 2, "Yes", C_BOX_FG, C_BOX_BG, yes_attr);
  c->yes_x1 = inner.x + cx;
  cx += draw_text(inner, cx, 2, "  ·  ", C_BOX_FG, C_BOX_BG, ATTR_DIM);
  c->no_x0 = inner.x + cx;
  cx += draw_text(inner, cx, 2, "No", C_BOX_FG, C_BOX_BG, no_attr);
  c->no_x1 = inner.x + cx;
  c->row_y = inner.y + 2;

  app_draw_bottom_hint(r, "←/→ + Enter, or click to select  ·  Esc cancel");
}

static Cmd_t answer(Confirm_t *c, int yes) {
  if (yes) {
    return c->on_yes != NULL ? c->on_yes(c->ctx) : (Cmd_t){CMD_POP, NULL};
  }
  return c->on_no != NULL ? c->on_no(c->ctx) : (Cmd_t){CMD_POP, NULL};
}

static Cmd_t confirm_handle(void *ctx, const Event_t *ev) {
  Confirm_t *c = (Confirm_t *)ctx;

  /* A click only moves the highlight, the same as an arrow key — Enter is the
   * one way to answer, so a stray click cannot agree to a draw. */
  if (ev->type == EV_MOUSE && ev->mouse.kind == MOUSE_PRESS && ev->mouse.button == 0) {
    if (ev->mouse.row == c->row_y && ev->mouse.col >= c->yes_x0 && ev->mouse.col < c->yes_x1) {
      c->selected = 0;
    } else if (ev->mouse.row == c->row_y && ev->mouse.col >= c->no_x0 &&
               ev->mouse.col < c->no_x1) {
      c->selected = 1;
    }
    return CMD_STAY;
  }
  if (ev->type != EV_KEY) {
    return CMD_STAY;
  }
  if (ev->key.name == KEY_ESCAPE) {
    return answer(c, 0);
  }
  if (ev->key.name == KEY_LEFT || ev->key.name == KEY_RIGHT || ev->key.name == KEY_UP ||
      ev->key.name == KEY_DOWN) {
    c->selected = 1 - c->selected;
    return CMD_STAY;
  }
  if (ev->key.name == KEY_ENTER) {
    return answer(c, c->selected == 0);
  }
  return CMD_STAY;
}

Screen *confirm_screen(const char *message, Cmd_t (*on_yes)(void *ctx), Cmd_t (*on_no)(void *ctx),
                        void *ctx) {
  static Screen screen;

  snprintf(g_confirm.message, sizeof(g_confirm.message), "%s", message);
  g_confirm.on_yes = on_yes;
  g_confirm.on_no = on_no;
  g_confirm.ctx = ctx;
  g_confirm.selected = CONFIRM_NO;

  screen.on_enter = confirm_on_enter;
  screen.on_exit = NULL;
  screen.handle = confirm_handle;
  screen.render = confirm_render;
  screen.ctx = &g_confirm;
  screen.opaque = 0;
  return &screen;
}
