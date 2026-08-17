#ifndef SAVE_H
#define SAVE_H

#include "types.h"

#include <stddef.h>

/* Where saved games collect — beside the binary; see design.md's open
 * question on this, deferrable, no requirement depends on the exact
 * location. */
#define SAVED_GAMES_DIR "games"

/* Creates the saved-games directory if it does not already exist. Returns 1
 * on success (including "already exists"), 0 on failure. */
int save_games_dir_ensure(void);

/* Builds a fresh path in the saved-games directory for a game that has never
 * been saved: the current date plus a random id, e.g.
 * "games/2026-08-16-a3f9c1.chess" — assigned once and then reused for every
 * later save of that same game (see GameState.save_path), so re-saving
 * updates one file instead of collecting a new one each time. Ensures the
 * directory exists. Returns 1 on success. */
int save_new_game_path(char *out_path, size_t out_len);

/* One entry in the saved-games list: where it lives, a human-readable
 * date/time label derived from the filename, and a summary read from the
 * file itself. readable is 0 if the file exists but could not be parsed —
 * shown rather than silently skipped, so a corrupted save is still visible
 * and nameable. */
typedef struct {
  char path[300];
  char label[24]; /* "YYYY-MM-DD HH:MM:SS" */
  int move_count;
  Color side_to_move;
  int readable;
} Saved_game_entry_t;

/* Lists every saved game in the saved-games directory into out (room for at
 * least max entries), most recent first. Returns the number written. */
int save_list_games(Saved_game_entry_t *out, int max);

/* Persistence as text: a FEN line for the starting position, followed by a
 * line of space-separated coordinate moves played from it.
 *
 *   rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
 *   e2e4 e7e5 g1f3 b8c6 f1b5
 *
 * Loading replays every move through the legal move generator, so a hand-
 * edited or corrupted file is caught at load rather than trusted. Nothing
 * here depends on struct layout, compiler, or machine byte order — only the
 * text format core/notation.h already defines.
 */

/* Why a read did not produce a game. NOT_A_SAVE_FILE covers a malformed
 * position and move text that does not parse as coordinate notation — both
 * mean the file is not a save file. ILLEGAL_MOVE is different: the text
 * parses but names a move that is not legal in the position it would be
 * played from, which points at move_number rather than at the file being
 * unrelated content. */
typedef enum {
  SAVE_READ_OK,
  SAVE_READ_NO_FILE,
  SAVE_READ_OLD_VERSION,
  SAVE_READ_NOT_A_SAVE_FILE,
  SAVE_READ_ILLEGAL_MOVE
} Save_read_status_t;

typedef struct {
  Save_read_status_t status;
  int move_number; /* 1-based; meaningful only for SAVE_READ_ILLEGAL_MOVE */
} Save_read_result_t;

/* Writes state's starting position and the moves played since (p_history_head)
 * to path. Written to path with a ".tmp" suffix and renamed into place, so an
 * interruption mid-write cannot corrupt whatever already exists at path.
 * Reports nothing itself: called from inside the alternate screen, so the
 * caller draws the outcome into a frame. Returns 1 on success. */
int save_write(const char *path, const GameState *state);

/* Reads path into *out, which is written only on SAVE_READ_OK; out's own
 * lists (captures, history, hashes) are freshly built and belong to the
 * caller, who must free (or take ownership of) whatever *out held before the
 * call if this call succeeds. Nothing is touched on failure. */
Save_read_result_t save_read(const char *path, GameState *out);

/* Whether a file exists at path. */
int save_file_exists(const char *path);

/* Writes a message suitable for the status bar or a toast into out, given a
 * read result — naming the move number for SAVE_READ_ILLEGAL_MOVE, so a
 * generator bug is not misreported as generic file corruption. */
void save_read_message(Save_read_result_t result, char *out, size_t out_len);

#endif /* SAVE_H */
