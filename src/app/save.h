#ifndef SAVE_H
#define SAVE_H

#include "types.h"

/* Persistence to game_save.bin.
 *
 * A raw dump of the in-memory structs, so the file format is tied to the
 * struct layout. Replaced by a text format in the app-shell-and-persistence
 * change.
 */

/* Size of the header's fixed-size, NUL-terminated build-version field.
 * Kept in step with SAVE_VERSION_FIELD_LEN in the Makefile, which refuses
 * to build a VERSION that would not fit here. */
#define SAVE_VERSION_LEN 16

int save_game(const GameState *p_state);

/* Loads the game into *p_state. If p_version_out is not NULL, the build
 * version recorded in the file is copied into it (a caller-owned buffer of
 * at least SAVE_VERSION_LEN bytes); it is never compared against the
 * running build's version, since a save is accepted or refused by its
 * format alone. */
int load_game(GameState *p_state, char *p_version_out);

#endif /* SAVE_H */
