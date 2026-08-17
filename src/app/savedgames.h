#ifndef SAVEDGAMES_H
#define SAVEDGAMES_H

#include "app/app.h"
#include "types.h"

/* The list of every saved game, for loading one from the main menu — any
 * number of them, each named by the date and time it was saved (see
 * app/save.h).
 *
 * on_loaded is called once a game is chosen and successfully read; it
 * returns the Cmd_t this screen itself returns, so the caller — not this
 * screen — decides what replaces it (typically CMD_REPLACE with the game
 * screen), the same contract the old SaveSlot overlay used. Escape pops
 * without calling it.
 */
Screen *savedgames_screen(Cmd_t (*on_loaded)(void *ctx, GameState loaded), void *ctx);

#endif /* SAVEDGAMES_H */
