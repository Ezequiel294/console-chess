#include "app/save.h"

#include "core/board.h"
#include "core/history.h"
#include "core/movegen.h"
#include "core/notation.h"
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
  if (after_moves != NULL) {
    *after_moves = '\0';
  }
  size_t moves_len = strlen(moves_line);
  if (moves_len > 0 && moves_line[moves_len - 1] == '\r') {
    moves_line[moves_len - 1] = '\0';
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

    make(&loaded.position, move);
    update_history(&loaded.p_history_head, from, to, move);
    push_hash(&loaded.p_hash_history_head, loaded.position.hash);
  }

  free(buf);
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
 * the same session, even the same second, do not collide; the timestamp
 * prefix makes an actual collision harder still. */
static void new_id(char out[7]) {
  snprintf(out, 7, "%06x", (unsigned)rand() & 0xFFFFFFu);
}

/* The filename is the timestamp plus the id —
 * "2026-08-16_140503-a3f9c1.chess" — sortable lexicographically in the same
 * order as chronologically despite the id suffix, since two saves distinct
 * enough to need the id apart also differ earlier in the string. */
int save_new_game_path(char *out_path, size_t out_len) {
  if (!save_games_dir_ensure()) {
    return 0;
  }

  time_t now = time(NULL);
  struct tm tm_buf;
  struct tm *tm = localtime_r(&now, &tm_buf);

  char id[7];
  new_id(id);

  char path[300];
  int n = snprintf(path, sizeof(path), "%s/%04d-%02d-%02d_%02d%02d%02d-%s.chess",
                    SAVED_GAMES_DIR, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
                    tm->tm_min, tm->tm_sec, id);
  if (n < 0 || (size_t)n >= sizeof(path)) {
    return 0;
  }
  snprintf(out_path, out_len, "%s", path);
  return 1;
}

/* "2026-08-16_140503-a3f9c1" (the filename's stem) -> "2026-08-16 14:05:03".
 * sscanf stops after the fields it names, so the "-a3f9c1" id suffix is
 * simply left unconsumed rather than needing its own pattern. Falls back to
 * the stem itself, unparsed, for a file this program did not name — still a
 * usable label, just not reformatted. */
static void label_from_stem(const char *stem, char *out, size_t out_len) {
  int y, mo, d, h, mi, s;
  if (sscanf(stem, "%4d-%2d-%2d_%2d%2d%2d", &y, &mo, &d, &h, &mi, &s) == 6) {
    snprintf(out, out_len, "%04d-%02d-%02d %02d:%02d:%02d", y, mo, d, h, mi, s);
  } else {
    snprintf(out, out_len, "%s", stem);
  }
}

static int compare_entries_newest_first(const void *a, const void *b) {
  const Saved_game_entry_t *ea = (const Saved_game_entry_t *)a;
  const Saved_game_entry_t *eb = (const Saved_game_entry_t *)b;
  return strcmp(eb->path, ea->path);
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
    label_from_stem(stem, e->label, sizeof(e->label));

    GameState scratch = {0};
    Save_read_result_t r = save_read(e->path, &scratch);
    if (r.status == SAVE_READ_OK) {
      e->readable = 1;
      e->side_to_move = scratch.position.side_to_move;
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
      e->side_to_move = NONE;
    }

    count++;
  }
  closedir(dir);

  qsort(out, (size_t)count, sizeof(*out), compare_entries_newest_first);
  return count;
}

int save_file_exists(const char *path) { return access(path, F_OK) == 0; }

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
