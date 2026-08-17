#ifndef MAINMENU_H
#define MAINMENU_H

#include "app/app.h"
#include "types.h"

/* The entry point: new game, resume, load a slot, help, settings, quit.
 *
 * Owns state by address, the same idiom game_screen uses — main.c holds the
 * one GameState and every screen that can start or replace a game borrows it,
 * so there is never a second copy to fall out of sync. Starting or resuming a
 * game pushes the game screen over this one; leaving a game without quitting
 * pops back to reveal it, state intact.
 */
Screen *mainmenu_screen(GameState *state);

#endif /* MAINMENU_H */
