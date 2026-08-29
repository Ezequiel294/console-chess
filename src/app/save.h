#ifndef SAVE_H
#define SAVE_H

#include "types.h"

#include <stddef.h>
#include <time.h>

/* Where saved games collect — beside the binary; see design.md's open
 * question on this, deferrable, no requirement depends on the exact
 * location. */
#define SAVED_GAMES_DIR "games"

/* A name is at most this many codepoints, buffered as four bytes each plus a
 * NUL — see GameState.name. */
#define SAVE_NAME_MAX_CODEPOINTS 32
#define SAVE_NAME_BUF_LEN 129

/* Six lowercase hex digits plus a NUL — see GameState.id. */
#define SAVE_ID_LEN 7

/* A game's status leads its filename (see save-naming's Filenames decision):
 * finished or in progress. Nothing else is a status — a file whose name does
 * not lead with one of these two words is treated as SAVE_STATUS_ONGOING,
 * its whole stem taken as the name (save_parse_stem). */
typedef enum { SAVE_STATUS_ONGOING, SAVE_STATUS_FINISHED } Save_status_t;

/* Creates the saved-games directory if it does not already exist. Returns 1
 * on success (including "already exists"), 0 on failure. */
int save_games_dir_ensure(void);

/* Builds "games/<status>_<name>.chess" verbatim: no numbering, no directory
 * creation, no validation. name is expected to already be trimmed and
 * validated (see save_name_trim, save_name_validate). Returns 1 on success. */
int save_build_path(char *out_path, size_t out_len, Save_status_t status, const char *name);

/* Splits stem (a filename with ".chess" already removed) at its first
 * underscore into status and name. A first field that is neither "ongoing"
 * nor "finished" — every file written before this change included — means
 * the whole stem is the name of an ongoing game. */
void save_parse_stem(const char *stem, Save_status_t *status, char *name_out, size_t name_out_len);

/* Trims leading and trailing spaces from name into out. A name of nothing but
 * spaces becomes empty. Does not touch anything else — non-space whitespace,
 * if any, is left alone. */
void save_name_trim(const char *name, char *out, size_t out_len);

/* Rejects a name containing '/', '\', or a control character, filling err
 * with which. Does not check length (the prompt enforces
 * SAVE_NAME_MAX_CODEPOINTS as the player types) and does not trim (the
 * caller trims before validating). Returns 1 if the name may be used as-is. */
int save_name_validate(const char *name, char *err, size_t err_len);

/* Today's date, YYYY-MM-DD — the name a game is given when the player
 * submits none. */
void save_today_name(char *out, size_t out_len);

/* Reads just the id line from the save file at path, without building a
 * whole GameState — the narrow "is this file our own game?" check numbering
 * and the single save entry point both need. Returns 1 if path exists and
 * carries an id (id_out is then set), 0 otherwise — including "no such
 * file", "unreadable", and "no id line" alike, all of which mean the same
 * thing here: this is not, or can no longer be shown to be, our file. */
int save_read_id(const char *path, char *id_out, size_t id_out_len);

/* Determines the path status/name should be written to: the plain
 * "games/<status>_<name>.chess" if nothing is there yet or what is there
 * already carries own_id, otherwise that path with the lowest free "(n)"
 * appended. own_id is the saving game's own id — pass its GameState.id,
 * which must already be assigned (see save_perform). Ensures the games
 * directory exists. Returns 1 on success. */
int save_target_path(char *out_path, size_t out_len, Save_status_t status, const char *name,
                      const char *own_id);

/* One entry in the saved-games list: where it lives, its status and name (both
 * read from the filename, not the file's contents — see save_parse_stem),
 * when it was last saved, and a summary read from the file itself. readable
 * is 0 if the file exists but could not be parsed — shown rather than
 * silently skipped, so a corrupted save is still visible and nameable. */
typedef struct {
  char path[300];
  Save_status_t status;
  char name[SAVE_NAME_BUF_LEN];
  time_t mtime;
  int move_count;
  int readable;
} Saved_game_entry_t;

/* Lists every saved game in the saved-games directory into out (room for at
 * least max entries), most recently saved first. Returns the number written. */
int save_list_games(Saved_game_entry_t *out, int max);

/* Persistence as text: a FEN line for the starting position, a line of
 * space-separated coordinate moves played from it, and then zero or more
 * "key value" lines recording whatever else is known about the game — see
 * design.md's "The file gains a keyed trailer, after the moves".
 *
 *   rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
 *   e2e4 e7e5 g1f3 b8c6 f1b5
 *   id 5d54b0
 *   name given
 *
 * Loading replays every move through the legal move generator, so a hand-
 * edited or corrupted file is caught at load rather than trusted. Nothing
 * here depends on struct layout, compiler, or machine byte order — only the
 * text format core/notation.h already defines.
 */

/* Why a read did not produce a game. NOT_A_SAVE_FILE covers a malformed
 * position, move text that does not parse as coordinate notation, and a
 * trailer that is not exactly the keys this format defines — all mean the
 * file is not a save file. ILLEGAL_MOVE is different: the text parses but
 * names a move that is not legal in the position it would be played from,
 * which points at move_number rather than at the file being unrelated
 * content. */
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

/* Writes state's starting position, the moves played since (p_history_head),
 * and its trailer (result if it has ended, id, and whether its name was
 * given) to path. Written to path with a ".tmp" suffix and renamed into
 * place, so an interruption mid-write cannot corrupt whatever already
 * exists at path. Reports nothing itself: called from inside the alternate
 * screen, so the caller draws the outcome into a frame. Returns 1 on
 * success. */
int save_write(const char *path, const GameState *state);

/* Reads path into *out, which is written only on SAVE_READ_OK; out's own
 * lists (captures, history, hashes) are freshly built and belong to the
 * caller, who must free (or take ownership of) whatever *out held before the
 * call if this call succeeds. Nothing is touched on failure.
 *
 * out's id, name_given, and result come from the trailer (absent means no
 * id yet, name auto, and in progress, respectively); out's name is left
 * empty; a name lives in the filename, not the file, so the caller fills it
 * in from the path (see save_parse_stem). */
Save_read_result_t save_read(const char *path, GameState *out);

/* Whether a file exists at path. */
int save_file_exists(const char *path);

/* Writes a message suitable for the status bar or a toast into out, given a
 * read result — naming the move number for SAVE_READ_ILLEGAL_MOVE, so a
 * generator bug is not misreported as generic file corruption. */
void save_read_message(Save_read_result_t result, char *out, size_t out_len);

/* The one save entry point — every save (the save command during play, the
 * quit picker, the result screen, a rename) goes through this. Determines
 * the name (an existing given name is kept; an auto name is recomputed from
 * today's date), builds and numbers the target path, verifies the
 * remembered file still carries this game's id (forgetting it if not),
 * writes, removes the file it superseded only after a successful write, and
 * updates state's remembered path, id, name, and name_given in place.
 *
 * name/name_given describe a name just supplied by the player — a first
 * save or a rename — already trimmed and validated by the caller; pass name
 * NULL to keep the game's existing name (recomputing today's date if it was
 * never given one). Assigns state->id if this is the game's first-ever save.
 *
 * Returns 1 on success and writes a status message into message; message may
 * be NULL. */
int save_perform(GameState *state, Save_status_t status, const char *name, int name_given,
                  char *message, size_t message_len);

#endif /* SAVE_H */
