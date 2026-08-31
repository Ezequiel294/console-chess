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

#define C_HINT 246

void app_draw_bottom_hint(Rect screen, const char *text) {
  if (screen.h < 1) {
    return;
  }
  draw_fill(rect_sub(screen, 0, screen.h - 1, screen.w, 1), ' ', COLOR_DEFAULT, COLOR_DEFAULT,
            ATTR_NONE);
  draw_text(screen, 0, screen.h - 1, text, C_HINT, COLOR_DEFAULT, ATTR_DIM);
}

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

/* How long the loop may wait before the next frame: the soonest request on
 * the stack, or -1 when nothing on it asks to be woken — in which case the
 * wait is a true indefinite block, doing no work and drawing no frame until
 * something happens. */
static int stack_wake_in_ms(void) {
  int soonest = -1;
  for (int i = 0; i < g_depth; i++) {
    Screen *screen = g_stack[i];
    if (screen->wake_in_ms == NULL) {
      continue;
    }
    int wake = screen->wake_in_ms(screen->ctx);
    if (wake < 0) {
      continue;
    }
    if (soonest < 0 || wake < soonest) {
      soonest = wake;
    }
  }
  return soonest;
}

/* Every screen that asked for it, top to bottom, with one reading of the
 * clock shared between them so two screens ticked in the same pass can never
 * disagree about what time it is. The clock is read only when something
 * actually wants a tick, so the idle path costs nothing. */
static void tick_stack(void) {
  int any = 0;
  for (int i = 0; i < g_depth; i++) {
    if (g_stack[i]->tick != NULL) {
      any = 1;
      break;
    }
  }
  if (!any) {
    return;
  }
  uint64_t now = term_now_ms();
  for (int i = g_depth - 1; i >= 0; i--) {
    if (g_stack[i]->tick != NULL) {
      g_stack[i]->tick(g_stack[i]->ctx, now);
    }
  }
}

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
  case CMD_POP_REPLACE:
    pop(); /* the overlay returning this command */
    pop(); /* the screen it was covering, now on top */
    push(cmd.screen);
    break;
  case CMD_RESET:
    while (g_depth > 0) {
      pop();
    }
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
    /* Before the frame, so what a tick changed is what the frame shows. */
    tick_stack();
    draw_frame(render_bounds());

    Event_t ev = input_next_within(stack_wake_in_ms());

    if (ev.type == EV_TIMEOUT) {
      /* Swallowed here and never dispatched: an expiry says nothing about
       * what the user did, and no screen's handle should have to know the
       * difference. What it is for is the tick at the top of this loop. */
      continue;
    }

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
