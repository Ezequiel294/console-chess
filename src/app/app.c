#include "app/app.h"

#include "ui/glyphs.h"
#include "ui/layout.h"
#include "ui/term.h"

#include <assert.h>
#include <stddef.h>

static Screen *g_stack[APP_STACK_MAX];
static int g_depth = 0;
static Screen *g_too_small = NULL;

void app_set_too_small_screen(Screen *screen) { g_too_small = screen; }

/* --- Stack -------------------------------------------------------------- */

static int push(Screen *screen) {
  if (screen == NULL || g_depth >= APP_STACK_MAX) {
    return 0;
  }
  g_stack[g_depth++] = screen;
  if (screen->on_enter != NULL) {
    screen->on_enter(screen->ctx);
  }
  return 1;
}

/* on_exit runs before the screen leaves the stack, so a screen releasing what
 * it owns is still a screen while it does so. */
static void pop(void) {
  if (g_depth == 0) {
    return;
  }
  Screen *screen = g_stack[--g_depth];
  if (screen->on_exit != NULL) {
    screen->on_exit(screen->ctx);
  }
}

static Screen *top(void) { return g_depth > 0 ? g_stack[g_depth - 1] : NULL; }

/* Returns 0 when the application should stop. */
static int apply(Cmd_t cmd) {
  switch (cmd.type) {
  case CMD_NONE:
    break;
  case CMD_PUSH:
    push(cmd.screen);
    break;
  case CMD_POP:
    pop();
    break;
  case CMD_REPLACE:
    pop();
    push(cmd.screen);
    break;
  case CMD_QUIT:
    return 0;
  }
  return g_depth > 0;
}

/* --- Frame -------------------------------------------------------------- */

static int terminal_fits(Rect bounds) {
  int width = glyphs_width();
  return bounds.w >= layout_min_cols(width) && bounds.h >= layout_min_rows(width);
}

static void draw_frame(Rect bounds) {
  render_begin();

  if (!terminal_fits(bounds) && g_too_small != NULL) {
    g_too_small->render(g_too_small->ctx, bounds);
    render_flush();
    return;
  }

  /* Start at the topmost screen that covers everything: nothing below it can
   * show through, and everything above it is an overlay that must. */
  int base = 0;
  for (int i = g_depth - 1; i >= 0; i--) {
    if (g_stack[i]->opaque) {
      base = i;
      break;
    }
  }

  for (int i = base; i < g_depth; i++) {
    /* The region is a parameter, never a field. This assert is the enforcement:
     * a screen that cached a size would be drawing against a stale rect, and
     * the rect a screen is given is always the whole current screen. */
    assert(bounds.x == 0 && bounds.y == 0);
    g_stack[i]->render(g_stack[i]->ctx, bounds);
  }

  render_flush();
}

/* --- Loop --------------------------------------------------------------- */

int app_run(Screen *initial) {
  Term_size_t size = term_size();
  if (!render_init(size.cols, size.rows)) {
    return 1;
  }
  if (!push(initial)) {
    return 1;
  }

  int running = 1;
  while (running) {
    draw_frame(render_bounds());

    Event_t ev = input_next();

    if (ev.type == EV_RESIZE) {
      Term_size_t now = term_size();
      /* Reallocating discards both grids, so the next frame is a full repaint.
       * That is correct — the old contents describe a screen that no longer
       * exists — and at one frame it is imperceptible. */
      if (!render_resize(now.cols, now.rows)) {
        return 1;
      }
      continue;
    }

    if (ev.type == EV_EOF) {
      break;
    }

    /* While the terminal is too small the game is not interactive: the stack
     * keeps its state untouched and only the too-small screen sees input. */
    Screen *target = terminal_fits(render_bounds()) ? top() : g_too_small;
    if (target == NULL || target->handle == NULL) {
      continue;
    }

    /* Only the screen on top. Screens beneath are covered and must not act on
     * a key the user aimed at the overlay. */
    running = apply(target->handle(target->ctx, &ev));
  }

  while (g_depth > 0) {
    pop();
  }
  return 0;
}
