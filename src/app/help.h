#ifndef HELP_H
#define HELP_H

#include "app/app.h"

/* Rules of the interface: how to move pieces, the command keys, and mouse
 * usage. A boxed overlay, pushed the same way from the main menu and from
 * mid-game — one screen, not a full-screen copy for the menu and an overlay
 * copy for play that could drift apart. Dismissing (Esc) returns to whatever
 * was beneath unchanged. */
Screen *help_screen(void);

#endif /* HELP_H */
