#ifndef HELP_H
#define HELP_H

#include "app/app.h"

/* Rules of the interface: how to move pieces, the command keys, and mouse
 * usage. A boxed overlay, pushed the same way from the main menu and from
 * mid-game — one screen, not a full-screen copy for the menu and an overlay
 * copy for play that could drift apart. Dismissing (Esc) returns to whatever
 * was beneath unchanged. */
Screen *help_screen(void);

/* The same overlay, describing a replay's own keys instead — stepping and
 * flipping, not moving a piece, so the two must never share one set of
 * lines. Pushed from a replay's own '?' key. */
Screen *replay_help_screen(void);

#endif /* HELP_H */
