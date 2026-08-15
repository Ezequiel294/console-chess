#ifndef SAVE_H
#define SAVE_H

#include "types.h"

/* Persistence to game_save.bin.
 *
 * The position is stored as FEN, since castling rights and the en passant
 * square have no representation in the old raw board dump — a save from
 * before this change cannot express them and is refused outright, not
 * reinterpreted. Captures and move history remain a raw dump of their linked
 * lists, as before. Replaced by a fuller text format in the
 * app-shell-and-persistence change.
 */

/* Size of the header's fixed-size, NUL-terminated build-version field.
 * Kept in step with SAVE_VERSION_FIELD_LEN in the Makefile, which refuses
 * to build a VERSION that would not fit here. */
#define SAVE_VERSION_LEN 16

/* Returns 1 on success, 0 on failure. Reports nothing itself: it is called from
 * inside the alternate screen, where a printed line lands wherever the cursor
 * happens to be and the frame that follows has no idea it is there. */
int save_game(const GameState *p_state);

/* Why a load did not happen. The reasons are distinct because they call for
 * different things from the player: write a save, use a different build, or
 * accept that this file is gone. */
typedef enum {
  LOAD_OK,
  LOAD_NO_FILE,
  LOAD_WRONG_FORMAT,
  LOAD_CORRUPT
} Load_result_t;

/* Loads the game into *p_state, which is written only on LOAD_OK and left
 * untouched otherwise. The lists it replaces belong to the caller, which must
 * take copies of the heads beforehand if it means to free them.
 *
 * If p_version_out is not NULL, the build version recorded in the file is
 * copied into it (a caller-owned buffer of at least SAVE_VERSION_LEN bytes);
 * it is never compared against the running build's version, since a save is
 * accepted or refused by its format alone. */
Load_result_t load_game(GameState *p_state, char *p_version_out);

#endif /* SAVE_H */
