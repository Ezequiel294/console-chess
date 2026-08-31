#ifndef NEWGAME_H
#define NEWGAME_H

#include "app/app.h"
#include "types.h"

/* The screen between choosing New Game and the board.
 *
 * A game has settings now, so there is one screen that asks for them and one
 * way a game comes into existence — reached from the main menu and from the
 * result screen alike. Leaving it without confirming starts nothing, ends
 * nothing, and discards nothing.
 *
 * Its state is an array of rows plus a selected index: a row is a label, a
 * list of options, a current index, and whether it currently applies. The
 * last row is the action that starts the game. Moving between rows and
 * changing the highlighted row's value are two distinct gestures, so
 * changing a setting can never start the game and moving between settings
 * can never change one.
 *
 * This is the shape a further per-game setting joins — notably whether the
 * opponent is another player at the same keyboard or a computer. Adding one
 * is adding an entry to that array: no gesture changes, no existing row
 * changes, and the path that starts a game does not change. Nothing is shown
 * for a setting that cannot yet be chosen, so the screen never promises
 * something that does not exist.
 *
 * on_start is called with the completed time control when the player
 * confirms — the same callback shape savedgames_screen uses for a loaded
 * game — and its Cmd_t is what this screen returns.
 */
Screen *newgame_screen(Cmd_t (*on_start)(void *ctx, Chess_clock_t clock), void *ctx);

#endif /* NEWGAME_H */
