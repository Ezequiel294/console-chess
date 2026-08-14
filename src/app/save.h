#ifndef SAVE_H
#define SAVE_H

#include "types.h"

/* Persistence to game_save.bin.
 *
 * A raw dump of the in-memory structs, so the file format is tied to the
 * struct layout. Replaced by a text format in the app-shell-and-persistence
 * change.
 */

int save_game(const GameState *p_state);
int load_game(GameState *p_state);

#endif /* SAVE_H */
