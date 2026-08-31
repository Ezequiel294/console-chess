#include "app/save.h"

#include "core/board.h"
#include "core/history.h"
#include "core/movegen.h"
#include "core/notation.h"
#include "core/outcome.h"
#include "core/position.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* The magic bytes every version of the old binary format opened with (see
 * save.c before this change). No legal FEN can start with any of these three
 * letters — piece placement uses only prnbqkPRNBQK12345678/ — so the check is
 * unambiguous. */
#define OLD_MAGIC "CCHS"
#define OLD_MAGIC_LEN 4

static int coord_syntax_ok(const char *tok) {
  size_t len = strlen(tok);
  if (len != 4 && len != 5) {
    return 0;
  }
  if (tok[0] < 'a' || tok[0] > 'h' || tok[1] < '1' || tok[1] > '8' ||
      tok[2] < 'a' || tok[2] > 'h' || tok[3] < '1' || tok[3] > '8') {
    return 0;
  }
  if (len == 5 && strchr("qrbn", tok[4]) == NULL) {
    return 0;
  }
  return 1;
}

/* Reads the whole file into a NUL-terminated buffer the caller must free, so
 * parsing works against a stable in-memory copy rather than juggling stdio
 * line buffers of an unknown save size. Returns NULL on any I/O error. */
static char *read_whole_file(FILE *file) {
  if (fseek(file, 0, SEEK_END) != 0) {
    return NULL;
  }
  long size = ftell(file);
  if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
    return NULL;
  }
  char *buf = (char *)malloc((size_t)size + 1);
  if (buf == NULL) {
    return NULL;
  }
  size_t n = fread(buf, 1, (size_t)size, file);
  buf[n] = '\0';
  return buf;
}

/* --- The trailer: result, id, name given/auto ---------------------------- */

static const char *reason_to_str(int reason) {
  switch ((Outcome_reason_t)reason) {
  case OUTCOME_CHECKMATE:
    return "checkmate";
  case OUTCOME_STALEMATE:
    return "stalemate";
  case OUTCOME_DRAW_FIFTY_MOVE:
    return "fifty-move";
  case OUTCOME_DRAW_INSUFFICIENT_MATERIAL:
    return "insufficient-material";
  case OUTCOME_DRAW_REPETITION:
    return "repetition";
  case OUTCOME_RESIGNATION:
    return "resignation";
  case OUTCOME_DRAW_AGREEMENT:
    return "agreement";
  case OUTCOME_TIMEOUT:
    return "timeout";
  case OUTCOME_DRAW_TIMEOUT_INSUFFICIENT_MATERIAL:
    return "timeout-insufficient-material";
  case OUTCOME_IN_PROGRESS:
    break;
  }
  return NULL;
}

static int str_to_reason(const char *s, int *out) {
  static const struct {
    const char *name;
    Outcome_reason_t reason;
  } TABLE[] = {
      {"checkmate", OUTCOME_CHECKMATE},
      {"stalemate", OUTCOME_STALEMATE},
      {"fifty-move", OUTCOME_DRAW_FIFTY_MOVE},
      {"insufficient-material", OUTCOME_DRAW_INSUFFICIENT_MATERIAL},
      {"repetition", OUTCOME_DRAW_REPETITION},
      {"resignation", OUTCOME_RESIGNATION},
      {"agreement", OUTCOME_DRAW_AGREEMENT},
      {"timeout", OUTCOME_TIMEOUT},
      {"timeout-insufficient-material", OUTCOME_DRAW_TIMEOUT_INSUFFICIENT_MATERIAL},
  };
  for (size_t i = 0; i < sizeof(TABLE) / sizeof(TABLE[0]); i++) {
    if (strcmp(s, TABLE[i].name) == 0) {
      *out = (int)TABLE[i].reason;
      return 1;
    }
  }
  return 0;
}

static const char *score_for_winner(Color winner) {
  if (winner == WHITE) {
    return "1-0";
  }
  if (winner == BLACK) {
    return "0-1";
  }
  return "1/2-1/2";
}

/* score is redundant with reason in every case but resignation, and is kept
 * because it is what makes the line legible to a human (see design.md). */
static int score_to_winner(const char *s, Color *out) {
  if (strcmp(s, "1-0") == 0) {
    *out = WHITE;
    return 1;
  }
  if (strcmp(s, "0-1") == 0) {
    *out = BLACK;
    return 1;
  }
  if (strcmp(s, "1/2-1/2") == 0) {
    *out = NONE;
    return 1;
  }
  return 0;
}

static int hex6(const char *s) {
  if (strlen(s) != 6) {
    return 0;
  }
  for (int i = 0; i < 6; i++) {
    char c = s[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
      return 0;
    }
  }
  return 1;
}

/* Parses the trailer — the text after the moves line, zero or more "key
 * value..." lines in any order — into the three things it can hold. Returns
 * 1 if every line was understood, 0 on an unknown key, a duplicate key, or a
 * malformed value, in which case the file is not a save file at all (see
 * save.h's SAVE_READ_NOT_A_SAVE_FILE). trailer is mutated (split on '\n'). */
static int parse_trailer(char *trailer, int *result_reason_out, Color *result_winner_out,
                          char *id_out, size_t id_out_len, int *name_given_out) {
  *result_reason_out = OUTCOME_IN_PROGRESS;
  *result_winner_out = NONE;
  id_out[0] = '\0';
  *name_given_out = 0;

  int result_seen = 0, id_seen = 0, name_seen = 0;

  char *p = trailer;
  while (p != NULL && *p != '\0') {
    char *line = p;
    char *nl = strchr(p, '\n');
    if (nl != NULL) {
      *nl = '\0';
      p = nl + 1;
    } else {
      p = NULL;
    }
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\r') {
      line[--len] = '\0';
    }
    if (len == 0) {
      continue; /* a trailing blank line from the file's final newline */
    }

    char *sp1 = strchr(line, ' ');
    if (sp1 == NULL) {
      return 0;
    }
    *sp1 = '\0';
    const char *key = line;
    char *value = sp1 + 1;

    if (strcmp(key, "result") == 0) {
      if (result_seen) {
        return 0;
      }
      char *sp2 = strchr(value, ' ');
      if (sp2 == NULL) {
        return 0;
      }
      *sp2 = '\0';
      const char *score = value;
      const char *reason = sp2 + 1;
      if (strchr(reason, ' ') != NULL) {
        return 0; /* more than two fields */
      }
      Color winner;
      int reason_val;
      if (!score_to_winner(score, &winner) || !str_to_reason(reason, &reason_val)) {
        return 0;
      }
      *result_reason_out = reason_val;
      *result_winner_out = winner;
      result_seen = 1;
    } else if (strcmp(key, "id") == 0) {
      if (id_seen || strchr(value, ' ') != NULL || !hex6(value)) {
        return 0;
      }
      snprintf(id_out, id_out_len, "%s", value);
      id_seen = 1;
    } else if (strcmp(key, "name") == 0) {
      if (name_seen || strchr(value, ' ') != NULL) {
        return 0;
      }
      if (strcmp(value, "given") == 0) {
        *name_given_out = 1;
      } else if (strcmp(value, "auto") == 0) {
        *name_given_out = 0;
      } else {
        return 0;
      }
      name_seen = 1;
    } else {
      return 0; /* an unknown key: a half-understood file is worse than a
                 * refused one */
    }
  }
  return 1;
}

int save_write(const char *path, const GameState *state) {
  char tmp_path[512];
  int n = snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
  if (n < 0 || (size_t)n >= sizeof(tmp_path)) {
    return 0;
  }

  FILE *file = fopen(tmp_path, "wb");
  if (file == NULL) {
    return 0;
  }

  char fen[FEN_MAX_LEN];
  fen_write(&state->start_position, fen);
  if (fprintf(file, "%s\n", fen) < 0) {
    fclose(file);
    remove(tmp_path);
    return 0;
  }

  int first = 1;
  for (const History_node_t *p = state->p_history_head; p != NULL; p = p->p_next) {
    char coord[COORD_MAX_LEN];
    move_to_coord(p->move, coord);
    if (fprintf(file, "%s%s", first ? "" : " ", coord) < 0) {
      fclose(file);
      remove(tmp_path);
      return 0;
    }
    first = 0;
  }
  if (fprintf(file, "\n") < 0) {
    fclose(file);
    remove(tmp_path);
    return 0;
  }

  if (state->result_reason != OUTCOME_IN_PROGRESS) {
    const char *reason = reason_to_str(state->result_reason);
    const char *score = score_for_winner(state->result_winner);
    if (fprintf(file, "result %s %s\n", score, reason) < 0) {
      fclose(file);
      remove(tmp_path);
      return 0;
    }
  }
  if (state->id[0] != '\0') {
    if (fprintf(file, "id %s\n", state->id) < 0) {
      fclose(file);
      remove(tmp_path);
      return 0;
    }
  }
  if (fprintf(file, "name %s\n", state->name_given ? "given" : "auto") < 0) {
    fclose(file);
    remove(tmp_path);
    return 0;
  }

  if (ferror(file) || fclose(file) != 0) {
    remove(tmp_path);
    return 0;
  }

  /* Atomic on the filesystems that matter: a crash or kill between here and
   * the write above leaves the old file at path untouched, never a
   * half-written one. */
  if (rename(tmp_path, path) != 0) {
    remove(tmp_path);
    return 0;
  }

  return 1;
}

Save_read_result_t save_read(const char *path, GameState *out) {
  Save_read_result_t fail_no_file = {SAVE_READ_NO_FILE, 0};
  Save_read_result_t fail_old = {SAVE_READ_OLD_VERSION, 0};
  Save_read_result_t fail_not_save = {SAVE_READ_NOT_A_SAVE_FILE, 0};

  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    return fail_no_file;
  }

  char *buf = read_whole_file(file);
  fclose(file);
  if (buf == NULL) {
    return fail_not_save;
  }

  if (strncmp(buf, OLD_MAGIC, OLD_MAGIC_LEN) == 0) {
    free(buf);
    return fail_old;
  }

  char *fen_line = buf;
  char *rest = strchr(buf, '\n');
  if (rest == NULL) {
    free(buf);
    return fail_not_save;
  }
  *rest = '\0';
  rest++;

  /* A save written on Windows or hand-edited may carry a trailing \r; strip
   * it from both lines so the FEN and each move token compare cleanly. */
  size_t fen_len = strlen(fen_line);
  if (fen_len > 0 && fen_line[fen_len - 1] == '\r') {
    fen_line[fen_len - 1] = '\0';
  }

  char *moves_line = rest;
  char *after_moves = strchr(moves_line, '\n');
  char *trailer = NULL;
  if (after_moves != NULL) {
    *after_moves = '\0';
    trailer = after_moves + 1;
  }
  size_t moves_len = strlen(moves_line);
  if (moves_len > 0 && moves_line[moves_len - 1] == '\r') {
    moves_line[moves_len - 1] = '\0';
  }

  int result_reason;
  Color result_winner;
  char id[SAVE_ID_LEN];
  int name_given;
  if (!parse_trailer(trailer, &result_reason, &result_winner, id, sizeof(id), &name_given)) {
    free(buf);
    return fail_not_save;
  }

  GameState loaded = {0};
  if (!fen_parse(fen_line, &loaded.start_position)) {
    free(buf);
    return fail_not_save;
  }
  loaded.position = loaded.start_position;
  push_hash(&loaded.p_hash_history_head, loaded.position.hash);

  int move_number = 0;
  char *p = moves_line;
  while (*p != '\0') {
    while (*p == ' ') {
      p++;
    }
    if (*p == '\0') {
      break;
    }
    char *tok = p;
    while (*p != '\0' && *p != ' ') {
      p++;
    }
    if (*p == ' ') {
      *p = '\0';
      p++;
    }

    move_number++;

    if (!coord_syntax_ok(tok)) {
      free(buf);
      free_captures(loaded.p_captures_white_head);
      free_captures(loaded.p_captures_black_head);
      free_history(loaded.p_history_head);
      free_hash_history(loaded.p_hash_history_head);
      return fail_not_save;
    }

    Move move;
    if (!coord_to_move(&loaded.position, tok, &move)) {
      free(buf);
      free_captures(loaded.p_captures_white_head);
      free_captures(loaded.p_captures_black_head);
      free_history(loaded.p_history_head);
      free_hash_history(loaded.p_hash_history_head);
      Save_read_result_t fail_illegal = {SAVE_READ_ILLEGAL_MOVE, move_number};
      return fail_illegal;
    }

    Color mover = loaded.position.side_to_move;
    char from[3], to[3];
    index_to_square(move.from_i, move.from_j, from);
    index_to_square(move.to_i, move.to_j, to);

    if (move.captured != FREE) {
      Color captured_color = (mover == WHITE) ? BLACK : WHITE;
      Captures_node_t **captures =
          (mover == WHITE) ? &loaded.p_captures_white_head : &loaded.p_captures_black_head;
      update_captures(captures, (Piece_t){.color = captured_color, .type = move.captured});
    }

    /* Zero until the format carries per-move readings — see the next
     * commit; an untimed game's history nodes carry zeros for good. */
    const int32_t node_remaining[2] = {0, 0};

    make(&loaded.position, move);
    update_history(&loaded.p_history_head, from, to, move, node_remaining);
    push_hash(&loaded.p_hash_history_head, loaded.position.hash);
  }

  free(buf);
  loaded.result_reason = result_reason;
  loaded.result_winner = result_winner;
  snprintf(loaded.id, sizeof(loaded.id), "%s", id);
  loaded.name_given = name_given;
  *out = loaded;
  Save_read_result_t ok = {SAVE_READ_OK, 0};
  return ok;
}

int save_games_dir_ensure(void) {
  if (mkdir(SAVED_GAMES_DIR, 0755) == 0) {
    return 1;
  }
  return errno == EEXIST;
}

/* Six lowercase hex digits from rand() — the caller (main.c) seeds it once at
 * startup. Not cryptographic, just distinct enough that two games saved in
 * the same session, even the same second, do not collide. */
static void new_id(char out[SAVE_ID_LEN]) {
  snprintf(out, SAVE_ID_LEN, "%06x", (unsigned)rand() & 0xFFFFFFu);
}

int save_build_path(char *out_path, size_t out_len, Save_status_t status, const char *name) {
  const char *status_str = (status == SAVE_STATUS_FINISHED) ? "finished" : "ongoing";
  int n = snprintf(out_path, out_len, "%s/%s_%s.chess", SAVED_GAMES_DIR, status_str, name);
  return n > 0 && (size_t)n < out_len;
}

void save_parse_stem(const char *stem, Save_status_t *status, char *name_out,
                      size_t name_out_len) {
  const char *underscore = strchr(stem, '_');
  if (underscore != NULL) {
    size_t status_len = (size_t)(underscore - stem);
    if (status_len == 7 && strncmp(stem, "ongoing", 7) == 0) {
      *status = SAVE_STATUS_ONGOING;
      snprintf(name_out, name_out_len, "%s", underscore + 1);
      return;
    }
    if (status_len == 8 && strncmp(stem, "finished", 8) == 0) {
      *status = SAVE_STATUS_FINISHED;
      snprintf(name_out, name_out_len, "%s", underscore + 1);
      return;
    }
  }
  /* Neither prefix: every file written before this change, and anything
   * hand-named — the whole stem is the name of an ongoing game. */
  *status = SAVE_STATUS_ONGOING;
  snprintf(name_out, name_out_len, "%s", stem);
}

void save_name_trim(const char *name, char *out, size_t out_len) {
  const char *start = name;
  while (*start == ' ') {
    start++;
  }
  const char *end = start + strlen(start);
  while (end > start && *(end - 1) == ' ') {
    end--;
  }
  size_t len = (size_t)(end - start);
  if (len >= out_len) {
    len = out_len - 1;
  }
  memcpy(out, start, len);
  out[len] = '\0';
}

int save_name_validate(const char *name, char *err, size_t err_len) {
  int has_slash = 0, has_control = 0;
  for (const unsigned char *p = (const unsigned char *)name; *p != '\0'; p++) {
    if (*p == '/' || *p == '\\') {
      has_slash = 1;
    } else if (*p < 0x20 || *p == 0x7F) {
      has_control = 1;
    }
  }
  if (!has_slash && !has_control) {
    return 1;
  }
  if (has_slash && has_control) {
    snprintf(err, err_len, "Names can't contain / \\ or control characters.");
  } else if (has_slash) {
    snprintf(err, err_len, "Names can't contain / or \\.");
  } else {
    snprintf(err, err_len, "Names can't contain control characters.");
  }
  return 0;
}

void save_today_name(char *out, size_t out_len) {
  time_t now = time(NULL);
  struct tm tm_buf;
  struct tm *tm = localtime_r(&now, &tm_buf);
  snprintf(out, out_len, "%04d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
}

int save_read_id(const char *path, char *id_out, size_t id_out_len) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    return 0;
  }
  char line[512];
  /* The FEN and moves lines, skipped unconditionally — the id, if there is
   * one, is always in the trailer after them. */
  if (fgets(line, sizeof(line), file) == NULL || fgets(line, sizeof(line), file) == NULL) {
    fclose(file);
    return 0;
  }
  int found = 0;
  while (fgets(line, sizeof(line), file) != NULL) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
      line[--len] = '\0';
    }
    if (strncmp(line, "id ", 3) == 0) {
      snprintf(id_out, id_out_len, "%s", line + 3);
      found = 1;
      break;
    }
  }
  fclose(file);
  return found;
}

int save_file_exists(const char *path) { return access(path, F_OK) == 0; }

int save_target_path(char *out_path, size_t out_len, Save_status_t status, const char *name,
                      const char *own_id) {
  if (!save_games_dir_ensure()) {
    return 0;
  }

  char base[300];
  if (!save_build_path(base, sizeof(base), status, name)) {
    return 0;
  }
  if (!save_file_exists(base)) {
    snprintf(out_path, out_len, "%s", base);
    return 1;
  }
  char existing_id[SAVE_ID_LEN];
  if (save_read_id(base, existing_id, sizeof(existing_id)) && strcmp(existing_id, own_id) == 0) {
    snprintf(out_path, out_len, "%s", base);
    return 1;
  }

  /* The number is part of the name from then on, computed here once and not
   * recomputed on later saves — see design.md's "Numbering is computed at
   * write time, against the directory". */
  for (int k = 1; k < 1000; k++) {
    char numbered_name[SAVE_NAME_BUF_LEN + 8];
    snprintf(numbered_name, sizeof(numbered_name), "%s(%d)", name, k);
    char candidate[300];
    if (!save_build_path(candidate, sizeof(candidate), status, numbered_name)) {
      return 0;
    }
    if (!save_file_exists(candidate)) {
      snprintf(out_path, out_len, "%s", candidate);
      return 1;
    }
    if (save_read_id(candidate, existing_id, sizeof(existing_id)) &&
        strcmp(existing_id, own_id) == 0) {
      snprintf(out_path, out_len, "%s", candidate);
      return 1;
    }
  }
  return 0;
}

int save_perform(GameState *state, Save_status_t status, const char *name, int name_given,
                  char *message, size_t message_len) {
  if (!save_games_dir_ensure()) {
    if (message != NULL) {
      snprintf(message, message_len, "Could not save.");
    }
    return 0;
  }

  if (state->id[0] == '\0') {
    new_id(state->id);
  }

  char final_name[SAVE_NAME_BUF_LEN];
  int final_given;
  if (name != NULL) {
    if (name_given) {
      snprintf(final_name, sizeof(final_name), "%s", name);
      final_given = 1;
    } else {
      save_today_name(final_name, sizeof(final_name));
      final_given = 0;
    }
  } else if (state->name_given && state->name[0] != '\0') {
    snprintf(final_name, sizeof(final_name), "%s", state->name);
    final_given = 1;
  } else {
    save_today_name(final_name, sizeof(final_name));
    final_given = 0;
  }

  char target[300];
  if (!save_target_path(target, sizeof(target), status, final_name, state->id)) {
    if (message != NULL) {
      snprintf(message, message_len, "Could not save.");
    }
    return 0;
  }

  int old_path_is_ours = 0;
  if (state->save_path[0] != '\0') {
    char existing_id[SAVE_ID_LEN];
    if (save_read_id(state->save_path, existing_id, sizeof(existing_id)) &&
        strcmp(existing_id, state->id) == 0) {
      old_path_is_ours = 1;
    } else {
      /* Deleted, replaced, or a different game: forget it and treat this as
       * a first save rather than overwriting whatever is there now. */
      state->save_path[0] = '\0';
    }
  }

  snprintf(state->name, sizeof(state->name), "%s", final_name);
  state->name_given = final_given;

  if (!save_write(target, state)) {
    if (message != NULL) {
      snprintf(message, message_len, "Could not save.");
    }
    return 0;
  }

  /* Removed only after a successful write, never before, so a failure above
   * leaves the previous save intact. */
  if (old_path_is_ours && strcmp(state->save_path, target) != 0) {
    remove(state->save_path);
  }
  snprintf(state->save_path, sizeof(state->save_path), "%s", target);

  if (message != NULL) {
    snprintf(message, message_len, "Saved.");
  }
  return 1;
}

static int compare_entries_newest_first(const void *a, const void *b) {
  const Saved_game_entry_t *ea = (const Saved_game_entry_t *)a;
  const Saved_game_entry_t *eb = (const Saved_game_entry_t *)b;
  if (eb->mtime > ea->mtime) {
    return 1;
  }
  if (eb->mtime < ea->mtime) {
    return -1;
  }
  return 0;
}

int save_list_games(Saved_game_entry_t *out, int max) {
  DIR *dir = opendir(SAVED_GAMES_DIR);
  if (dir == NULL) {
    return 0;
  }

  int count = 0;
  struct dirent *entry;
  while (count < max && (entry = readdir(dir)) != NULL) {
    const char *name = entry->d_name;
    size_t len = strlen(name);
    const char *ext = ".chess";
    size_t ext_len = strlen(ext);
    if (len <= ext_len || strcmp(name + len - ext_len, ext) != 0) {
      continue;
    }

    Saved_game_entry_t *e = &out[count];
    snprintf(e->path, sizeof(e->path), "%s/%s", SAVED_GAMES_DIR, name);

    char stem[256];
    size_t stem_len = len - ext_len;
    if (stem_len >= sizeof(stem)) {
      stem_len = sizeof(stem) - 1;
    }
    memcpy(stem, name, stem_len);
    stem[stem_len] = '\0';
    save_parse_stem(stem, &e->status, e->name, sizeof(e->name));

    struct stat st;
    e->mtime = (stat(e->path, &st) == 0) ? st.st_mtime : 0;

    GameState scratch = {0};
    Save_read_result_t r = save_read(e->path, &scratch);
    if (r.status == SAVE_READ_OK) {
      e->readable = 1;
      e->move_count = 0;
      for (const History_node_t *p = scratch.p_history_head; p != NULL; p = p->p_next) {
        e->move_count++;
      }
      free_captures(scratch.p_captures_white_head);
      free_captures(scratch.p_captures_black_head);
      free_history(scratch.p_history_head);
      free_hash_history(scratch.p_hash_history_head);
    } else {
      e->readable = 0;
      e->move_count = 0;
    }

    count++;
  }
  closedir(dir);

  qsort(out, (size_t)count, sizeof(*out), compare_entries_newest_first);
  return count;
}

void save_read_message(Save_read_result_t result, char *out, size_t out_len) {
  switch (result.status) {
  case SAVE_READ_OK:
    snprintf(out, out_len, "Game loaded.");
    return;
  case SAVE_READ_NO_FILE:
    snprintf(out, out_len, "No saved game found.");
    return;
  case SAVE_READ_OLD_VERSION:
    snprintf(out, out_len, "That save was written by an earlier, incompatible version.");
    return;
  case SAVE_READ_NOT_A_SAVE_FILE:
    snprintf(out, out_len, "That file is not a valid save.");
    return;
  case SAVE_READ_ILLEGAL_MOVE:
    snprintf(out, out_len, "That save has an illegal move at move %d and cannot be loaded.",
             result.move_number);
    return;
  }
  snprintf(out, out_len, "Could not load the saved game.");
}
