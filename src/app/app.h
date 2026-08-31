#ifndef APP_H
#define APP_H

#include "ui/input.h"
#include "ui/render.h"

#include <stdint.h>

/* The screen stack and the loop that drives it.
 *
 * A screen is a vtable and a context pointer. It is handed the region it may
 * draw into on every render and stores no size of its own, so resize handling
 * is not something each screen implements — it is something no screen can get
 * wrong. Every screen added by a later change is resize-correct on the day it
 * is written.
 *
 * A screen never touches the stack. It returns the transition it wants, and the
 * loop applies it after the screen has finished handling the event. A screen
 * that popped itself and then kept executing would be reading freed state;
 * returning a value instead makes that inexpressible.
 */

typedef struct Screen Screen;

typedef enum {
  CMD_NONE,
  CMD_PUSH,
  CMD_POP,
  CMD_REPLACE,
  /* Pops the screen returning this command (typically an overlay, e.g. a
   * confirmation) and then replaces the screen it was covering — the two
   * pops and one push a single Cmd_t cannot otherwise express. Used when an
   * overlay's own choice ends the game underneath it (resigning, accepting a
   * draw): without this, the screen beneath has to notice on its own next
   * event, one extra keypress away. */
  CMD_POP_REPLACE,
  /* Clears the whole screen stack and pushes one screen as the new root —
   * "back to the start", regardless of how deep the stack got (e.g. a game
   * opened from the saved-games list, so a raw pop would reveal that list
   * instead of the main menu). */
  CMD_RESET,
  CMD_QUIT
} Cmd_type_t;

typedef struct {
  Cmd_type_t type;
  Screen *screen; /* for CMD_PUSH and CMD_REPLACE */
} Cmd_t;

struct Screen {
  void (*on_enter)(void *ctx);
  void (*on_exit)(void *ctx);
  Cmd_t (*handle)(void *ctx, const Event_t *ev);
  void (*render)(void *ctx, Rect r);
  void *ctx;
  /* Whether this screen covers the display entirely. Rendering starts at the
   * topmost opaque screen, so an overlay is drawn over what is beneath it. */
  int opaque;

  /* Being woken by time rather than by input. Both are optional and NULL by
   * default, so a screen that does not ask for them behaves exactly as it
   * did before either existed.
   *
   * wake_in_ms says how soon this screen needs waking, in milliseconds, or
   * -1 for "not on my account". The loop waits no longer than the soonest
   * request across the whole stack, and blocks indefinitely when nobody
   * asks — a menu, a replay, or an untimed game does no work and draws no
   * frame until something happens.
   *
   * tick says that time has passed, and is handed the same monotonic
   * reading (see term_now_ms) every screen on the stack gets for that pass.
   * Unlike handle, it reaches screens beneath an overlay as well as the top
   * one: input is a statement about what the user did, and only the top
   * screen may act on it, but a tick is a statement about the world, and the
   * world does not stop for a covered screen — a game under a promotion
   * picker is still a game whose clock is running.
   *
   * It returns nothing, deliberately: a screen that is not on top must not
   * be able to move the stack. A screen whose state changed such that it
   * should be left says so on the next event it handles, the way the game
   * screen already does for an ending discovered under an overlay. */
  int (*wake_in_ms)(void *ctx);
  void (*tick)(void *ctx, uint64_t now_ms);
};

/* Chess never nests more than three deep. Fixed and statically allocated: a
 * push allocates nothing and cannot fail for want of memory. */
#define APP_STACK_MAX 8

/* The common case, spelled once. */
#define CMD_STAY ((Cmd_t){CMD_NONE, NULL})

/* Shown in place of the whole stack while the terminal is too small to lay the
 * game out. The stack is left untouched, so the game is exactly where it was
 * when the space comes back. */
void app_set_too_small_screen(Screen *screen);

/* The one hint line, on the screen's last row.
 *
 * Every screen puts its "how do I drive this" text here and nowhere else, so
 * there is exactly one place to read it — an overlay does not repeat inside
 * its own box what the bottom row is already saying. Overlays are drawn after
 * the screen beneath them (see draw_frame), so an overlay calling this
 * replaces that screen's hint for as long as it is up: the resignation picker
 * says "Enter confirm" where the game screen was listing its command keys.
 *
 * The row is cleared first, because draw_text only touches the columns it
 * writes and the line being replaced is usually the longer of the two. */
void app_draw_bottom_hint(Rect screen, const char *text);

/* Runs until a screen quits or input ends. Returns 0 on a clean exit. */
int app_run(Screen *initial);

#endif /* APP_H */
