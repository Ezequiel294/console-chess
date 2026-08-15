#ifndef APP_H
#define APP_H

#include "ui/input.h"
#include "ui/render.h"

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

/* Runs until a screen quits or input ends. Returns 0 on a clean exit. */
int app_run(Screen *initial);

#endif /* APP_H */
