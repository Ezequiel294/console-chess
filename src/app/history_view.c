#include "app/history_view.h"

#include "core/movegen.h"
#include "core/notation.h"
#include "ui/render.h"

#include <stdio.h>

#define C_BOX_BG 236
#define C_BOX_FG 250
#define C_LABEL 246

#define MAX_PLIES 1024

typedef struct {
  const GameState *state;
  int mark_current; /* whether to mark and scroll to the current move at all */
  char san[MAX_PLIES][SAN_MAX_LEN];
  int ply_count;
  /* Index into san of the move the board is currently showing (the last node
   * of p_history_head), or -1 to mark nothing — the starting position, or
   * live play, where mark_current is always false. */
  int current_ply;
  int scroll_row; /* topmost visible numbered-pair row */
} History_view_t;

static History_view_t g_history_view;

/* SAN over the whole game — history ++ redo — so a replay lists every move
 * regardless of how far it has stepped back; in live play p_redo_head is
 * always empty, so this is exactly the moves played so far. */
static void compute_san(History_view_t *h, int mark_current) {
  Position pos = h->state->start_position;
  int n = 0;
  for (const History_node_t *p = h->state->p_history_head; p != NULL && n < MAX_PLIES;
       p = p->p_next, n++) {
    move_to_san(&pos, p->move, h->san[n]);
    make(&pos, p->move);
  }
  h->current_ply = (mark_current && n > 0) ? n - 1 : -1;
  for (const History_node_t *p = h->state->p_redo_head; p != NULL && n < MAX_PLIES;
       p = p->p_next, n++) {
    move_to_san(&pos, p->move, h->san[n]);
    make(&pos, p->move);
  }
  h->ply_count = n;
}

static int pair_count(const History_view_t *h) { return (h->ply_count + 1) / 2; }

static void clamp_scroll(History_view_t *h, int visible_rows) {
  int total = pair_count(h);
  int max_scroll = total > visible_rows ? total - visible_rows : 0;
  if (h->scroll_row > max_scroll) {
    h->scroll_row = max_scroll;
  }
  if (h->scroll_row < 0) {
    h->scroll_row = 0;
  }
}

static void history_view_on_enter(void *ctx) {
  History_view_t *h = (History_view_t *)ctx;
  compute_san(h, h->mark_current);
  /* Scrolled to the bottom in live play, or to the marked move in a replay —
   * clamped once the visible row count is known, at first render. */
  h->scroll_row = (h->current_ply >= 0) ? h->current_ply / 2 : pair_count(h);
}

static void history_view_render(void *ctx, Rect r) {
  History_view_t *h = (History_view_t *)ctx;

  draw_fill(r, ' ', COLOR_DEFAULT, C_BOX_BG, ATTR_NONE);
  draw_text(r, 1, 0, "Move History", C_BOX_FG, C_BOX_BG, ATTR_BOLD);
  /* PgUp/PgDn/Home/End and the wheel all scroll too (see
   * history_view_handle); the hint names the two keys that matter rather than
   * listing every one that works. */
  draw_text(r, 1, r.h - 1, "↑/↓ or scroll  ·  Esc close", C_LABEL, C_BOX_BG, ATTR_DIM);

  Rect list = rect_sub(r, 1, 2, r.w - 2, r.h - 3);
  if (list.h < 1) {
    return;
  }

  if (h->ply_count == 0) {
    draw_text(list, 0, 0, "No moves played yet.", C_LABEL, C_BOX_BG, ATTR_NONE);
    return;
  }

  clamp_scroll(h, list.h);
  int total = pair_count(h);

  for (int row = 0; row < list.h && h->scroll_row + row < total; row++) {
    int k = h->scroll_row + row;
    int white_ply = 2 * k;
    int black_ply = 2 * k + 1;
    /* A marked ply is bracketed rather than distinguished by colour alone, so
     * it still reads in monochrome mode. */
    char white_field[SAN_MAX_LEN + 2];
    snprintf(white_field, sizeof(white_field), (white_ply == h->current_ply) ? "[%s]" : "%s",
             h->san[white_ply]);
    char line[64];
    if (black_ply < h->ply_count) {
      char black_field[SAN_MAX_LEN + 2];
      snprintf(black_field, sizeof(black_field), (black_ply == h->current_ply) ? "[%s]" : "%s",
               h->san[black_ply]);
      snprintf(line, sizeof(line), "%3d. %-9s %-9s", k + 1, white_field, black_field);
    } else {
      snprintf(line, sizeof(line), "%3d. %-9s", k + 1, white_field);
    }
    draw_text(list, 0, row, line, COLOR_DEFAULT, C_BOX_BG, ATTR_NONE);
  }
}

static Cmd_t history_view_handle(void *ctx, const Event_t *ev) {
  History_view_t *h = (History_view_t *)ctx;

  if (ev->type == EV_MOUSE && ev->mouse.kind == MOUSE_WHEEL) {
    /* The upper bound depends on how many rows actually fit, which render
     * alone knows; clamp_scroll runs there every frame. Only the lower bound
     * is enforced here, so a fast scroll up cannot leave scroll_row negative
     * before the next render clamps it. */
    h->scroll_row -= ev->mouse.wheel;
    if (h->scroll_row < 0) {
      h->scroll_row = 0;
    }
    return CMD_STAY;
  }
  if (ev->type != EV_KEY) {
    return CMD_STAY;
  }
  if (ev->key.name == KEY_ESCAPE || (ev->key.name == KEY_CHAR && ev->key.ch == 'H')) {
    return (Cmd_t){CMD_POP, NULL};
  }
  int total = pair_count(h);
  switch (ev->key.name) {
  case KEY_UP:
    h->scroll_row--;
    break;
  case KEY_DOWN:
    h->scroll_row++;
    break;
  case KEY_PAGE_UP:
    h->scroll_row -= 10;
    break;
  case KEY_PAGE_DOWN:
    h->scroll_row += 10;
    break;
  case KEY_HOME:
    h->scroll_row = 0;
    break;
  case KEY_END:
    h->scroll_row = total;
    break;
  default:
    break;
  }
  if (h->scroll_row < 0) {
    h->scroll_row = 0;
  }
  return CMD_STAY;
}

Screen *history_view_screen(const GameState *state, int mark_current) {
  static Screen screen;

  g_history_view.state = state;
  g_history_view.mark_current = mark_current;
  g_history_view.ply_count = 0;
  g_history_view.current_ply = -1;
  g_history_view.scroll_row = 0;

  screen.on_enter = history_view_on_enter;
  screen.on_exit = NULL;
  screen.handle = history_view_handle;
  screen.render = history_view_render;
  screen.ctx = &g_history_view;
  screen.opaque = 1;
  return &screen;
}
