#include "test.h"

#include "app/save.h"
#include "core/board.h"
#include "core/history.h"
#include "core/notation.h"
#include "core/outcome.h"
#include "core/position.h"

#include <stdio.h>
#include <string.h>

#define TMP_A "test_save_tmp_a.chess"
#define TMP_B "test_save_tmp_b.chess"

static void write_raw(const char *path, const char *contents) {
  FILE *f = fopen(path, "wb");
  fwrite(contents, 1, strlen(contents), f);
  fclose(f);
}

static int history_length(const History_node_t *head) {
  int n = 0;
  for (; head != NULL; head = head->p_next) {
    n++;
  }
  return n;
}

static void free_state(GameState *s) {
  free_captures(s->p_captures_white_head);
  free_captures(s->p_captures_black_head);
  free_history(s->p_history_head);
  free_hash_history(s->p_hash_history_head);
  free_history(s->p_redo_head);
  *s = (GameState){0};
}

static void check_ok_fixture(const char *contents, int expect_moves,
                              const char *expect_fen_of_final) {
  write_raw(TMP_A, contents);
  GameState state = {0};
  Save_read_result_t r = save_read(TMP_A, &state);
  TEST_CHECK_MSG(r.status == SAVE_READ_OK, "expected OK, got status %d for: %s", r.status,
                 contents);
  if (r.status != SAVE_READ_OK) {
    remove(TMP_A);
    return;
  }
  TEST_CHECK_MSG(history_length(state.p_history_head) == expect_moves,
                 "expected %d moves, got %d", expect_moves, history_length(state.p_history_head));

  if (expect_fen_of_final != NULL) {
    char fen[FEN_MAX_LEN];
    fen_write(&state.position, fen);
    TEST_CHECK_MSG(strcmp(fen, expect_fen_of_final) == 0, "final position mismatch: %s != %s",
                   fen, expect_fen_of_final);
  }

  /* Round trip: write it back out and read it again; the result must match
   * exactly, including every field FEN carries and the full move list. */
  TEST_CHECK(save_write(TMP_B, &state));
  GameState reloaded = {0};
  Save_read_result_t r2 = save_read(TMP_B, &reloaded);
  TEST_CHECK(r2.status == SAVE_READ_OK);
  if (r2.status == SAVE_READ_OK) {
    TEST_CHECK(memcmp(state.position.board, reloaded.position.board,
                       sizeof(state.position.board)) == 0);
    TEST_CHECK(state.position.side_to_move == reloaded.position.side_to_move);
    TEST_CHECK(state.position.castling_rights == reloaded.position.castling_rights);
    TEST_CHECK(state.position.ep_i == reloaded.position.ep_i &&
               state.position.ep_j == reloaded.position.ep_j);
    TEST_CHECK(state.position.halfmove_clock == reloaded.position.halfmove_clock);
    TEST_CHECK(state.position.fullmove_number == reloaded.position.fullmove_number);
    TEST_CHECK(history_length(state.p_history_head) == history_length(reloaded.p_history_head));
  }

  free_state(&state);
  free_state(&reloaded);
  remove(TMP_A);
  remove(TMP_B);
}

static void test_round_trips(void) {
  /* Plain game, no special moves. */
  check_ok_fixture("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                    "e2e4 e7e5 g1f3 b8c6\n",
                    4, NULL);

  /* Castling: e1g1 is White castling kingside. */
  check_ok_fixture("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1\n"
                    "e1g1\n",
                    1, "r3k2r/8/8/8/8/8/8/R4RK1 b kq - 1 1");

  /* En passant: e5d6 captures the pawn on d5, not the destination square. */
  check_ok_fixture("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1\n"
                    "e5d6\n",
                    1, "4k3/8/3P4/8/8/8/8/4K3 b - - 0 1");

  /* Promotion to a queen. */
  check_ok_fixture("8/4P3/8/8/4k3/8/8/4K3 w - - 0 1\n"
                    "e7e8q\n",
                    1, "4Q3/8/8/8/4k3/8/8/4K3 b - - 0 1");

  /* Checkmate: the fastest possible game, "fool's mate". */
  check_ok_fixture("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                    "f2f3 e7e5 g2g4 d8h4\n",
                    4, NULL);

  /* A file with no moves at all: still a legal (empty) save. */
  check_ok_fixture("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n\n", 0,
                    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

  /* Externally authored position: a valid FEN from other chess software, not
   * written by this program at all, loads and play may continue from it. */
  check_ok_fixture("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1\n\n", 0,
                    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
}

static void check_rejected(const char *contents, Save_read_status_t expect_status,
                            int expect_move_number) {
  write_raw(TMP_A, contents);

  /* Poisoned so a partial write on rejection would be caught. */
  GameState state;
  memset(&state, 0xAB, sizeof(state));
  GameState before = state;

  Save_read_result_t r = save_read(TMP_A, &state);
  TEST_CHECK_MSG(r.status == expect_status, "expected status %d, got %d for: %s", expect_status,
                 r.status, contents);
  if (expect_move_number >= 0) {
    TEST_CHECK_MSG(r.move_number == expect_move_number, "expected move_number %d, got %d",
                   expect_move_number, r.move_number);
  }
  TEST_CHECK_MSG(memcmp(&state, &before, sizeof(state)) == 0,
                 "save_read wrote a partial game on rejection: %s", contents);

  /* A rejected file is never deleted. */
  TEST_CHECK(save_file_exists(TMP_A));
  remove(TMP_A);
}

static void test_rejections(void) {
  /* Not a save file at all. */
  check_rejected("this is not a chess save file\n", SAVE_READ_NOT_A_SAVE_FILE, -1);
  check_rejected("", SAVE_READ_NOT_A_SAVE_FILE, -1);

  /* Malformed position. */
  check_rejected("not-a-fen w KQkq - 0 1\nmoves\n", SAVE_READ_NOT_A_SAVE_FILE, -1);

  /* Move text that does not parse as coordinate notation. */
  check_rejected("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                  "e2e4 not-a-move\n",
                  SAVE_READ_NOT_A_SAVE_FILE, -1);

  /* Syntactically valid coordinate text naming a move that is not legal:
   * distinguished from "not a save file" and points at move 2. */
  check_rejected("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                  "e2e4 e7e6 e4e6\n",
                  SAVE_READ_ILLEGAL_MOVE, 3);

  /* A legal-looking first move that is not actually legal in the position it
   * would be played from. */
  check_rejected("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                  "e2e5\n",
                  SAVE_READ_ILLEGAL_MOVE, 1);

  /* The old binary format's magic bytes: rejected as an earlier version, not
   * misread as text. */
  check_rejected("CCHS\x04\x00\x00\x00garbage", SAVE_READ_OLD_VERSION, -1);
}

static void test_trailer(void) {
  /* A finished game's result and reason round-trip, including resignation,
   * which (like an agreed draw) leaves no trace in the moves themselves. */
  write_raw(TMP_A, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                    "e2e4 e7e5\n"
                    "result 1-0 resignation\n"
                    "id 5d54b0\n"
                    "name given\n");
  GameState state = {0};
  Save_read_result_t r = save_read(TMP_A, &state);
  TEST_CHECK(r.status == SAVE_READ_OK);
  TEST_CHECK(state.result_reason == OUTCOME_RESIGNATION);
  TEST_CHECK(state.result_winner == WHITE);
  TEST_CHECK_MSG(strcmp(state.id, "5d54b0") == 0, "id mismatch: %s", state.id);
  TEST_CHECK(state.name_given == 1);
  free_state(&state);
  remove(TMP_A);

  /* An agreed draw, with no id and no name line (both absent, so auto). */
  write_raw(TMP_A, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                    "\n"
                    "result 1/2-1/2 agreement\n");
  GameState state2 = {0};
  Save_read_result_t r2 = save_read(TMP_A, &state2);
  TEST_CHECK(r2.status == SAVE_READ_OK);
  TEST_CHECK(state2.result_reason == OUTCOME_DRAW_AGREEMENT);
  TEST_CHECK(state2.result_winner == NONE);
  TEST_CHECK(state2.id[0] == '\0');
  TEST_CHECK(state2.name_given == 0);
  free_state(&state2);
  remove(TMP_A);

  /* A two-line file — no trailer at all — still loads, as in progress. */
  write_raw(TMP_A, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                    "e2e4 e7e5\n");
  GameState state3 = {0};
  Save_read_result_t r3 = save_read(TMP_A, &state3);
  TEST_CHECK(r3.status == SAVE_READ_OK);
  TEST_CHECK(state3.result_reason == OUTCOME_IN_PROGRESS);
  TEST_CHECK(state3.id[0] == '\0');
  TEST_CHECK(state3.name_given == 0);
  free_state(&state3);
  remove(TMP_A);

  /* Round trip through save_write and back: a finished game's id, whether
   * its name was given, and its recorded result all survive. */
  write_raw(TMP_A, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                    "f2f3 e7e5 g2g4 d8h4\n"
                    "result 0-1 checkmate\n"
                    "id abcdef\n"
                    "name given\n");
  GameState rt = {0};
  TEST_CHECK(save_read(TMP_A, &rt).status == SAVE_READ_OK);
  TEST_CHECK(save_write(TMP_B, &rt));
  GameState rt2 = {0};
  Save_read_result_t rtr = save_read(TMP_B, &rt2);
  TEST_CHECK(rtr.status == SAVE_READ_OK);
  TEST_CHECK(rt2.result_reason == OUTCOME_CHECKMATE);
  TEST_CHECK(rt2.result_winner == BLACK);
  TEST_CHECK(strcmp(rt2.id, "abcdef") == 0);
  TEST_CHECK(rt2.name_given == 1);
  free_state(&rt);
  free_state(&rt2);
  remove(TMP_A);
  remove(TMP_B);
}

static void test_trailer_rejections(void) {
  /* An unknown trailer key: a half-understood file is worse than a refused
   * one. */
  check_rejected("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                  "e2e4\n"
                  "bogus value\n",
                  SAVE_READ_NOT_A_SAVE_FILE, -1);

  /* A result whose score is none of the three the format defines. */
  check_rejected("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                  "e2e4\n"
                  "result 2-0 checkmate\n",
                  SAVE_READ_NOT_A_SAVE_FILE, -1);

  /* A result whose reason is none of the seven the format defines. */
  check_rejected("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                  "e2e4\n"
                  "result 1-0 bogus\n",
                  SAVE_READ_NOT_A_SAVE_FILE, -1);

  /* An id that is not six hex digits. */
  check_rejected("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                  "e2e4\n"
                  "id nothex\n",
                  SAVE_READ_NOT_A_SAVE_FILE, -1);

  /* A name value that is neither given nor auto. */
  check_rejected("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                  "e2e4\n"
                  "name sometimes\n",
                  SAVE_READ_NOT_A_SAVE_FILE, -1);

  /* A duplicate key. */
  check_rejected("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                  "e2e4\n"
                  "id abcdef\n"
                  "id 123456\n",
                  SAVE_READ_NOT_A_SAVE_FILE, -1);
}

static void test_save_naming(void) {
  /* Names with underscores, spaces, parentheses, and non-ASCII round-trip
   * through build-then-parse: everything after the first underscore is the
   * name, whatever it contains. */
  const char *names[] = {
      "Sicilian_Defense",
      "My Great Game",
      "Rematch(2)",
      "Caf\xc3\xa9 du Roi",
  };
  for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
    char path[300];
    TEST_CHECK(save_build_path(path, sizeof(path), SAVE_STATUS_ONGOING, names[i]));

    const char *base = strrchr(path, '/');
    TEST_CHECK(base != NULL);
    if (base == NULL) {
      continue;
    }
    base++;
    size_t len = strlen(base);
    TEST_CHECK(len > 6 && strcmp(base + len - 6, ".chess") == 0);
    char stem[256];
    memcpy(stem, base, len - 6);
    stem[len - 6] = '\0';

    Save_status_t status;
    char name[SAVE_NAME_BUF_LEN];
    save_parse_stem(stem, &status, name, sizeof(name));
    TEST_CHECK(status == SAVE_STATUS_ONGOING);
    TEST_CHECK_MSG(strcmp(name, names[i]) == 0, "expected %s, got %s", names[i], name);
  }

  /* An old-shape stem — no ongoing_/finished_ prefix — parses as ongoing,
   * name the whole stem: what keeps files from before this change listed. */
  {
    Save_status_t status;
    char name[SAVE_NAME_BUF_LEN];
    save_parse_stem("2026-08-16_140503-a3f9c1", &status, name, sizeof(name));
    TEST_CHECK(status == SAVE_STATUS_ONGOING);
    TEST_CHECK(strcmp(name, "2026-08-16_140503-a3f9c1") == 0);
  }

  /* Only the first underscore splits status from name, so a name that itself
   * looks like a status word is untouched. */
  {
    Save_status_t status;
    char name[SAVE_NAME_BUF_LEN];
    save_parse_stem("finished_ongoing_game", &status, name, sizeof(name));
    TEST_CHECK(status == SAVE_STATUS_FINISHED);
    TEST_CHECK(strcmp(name, "ongoing_game") == 0);
  }
}

static void test_numbering(void) {
  save_games_dir_ensure();

  const char *name = "test_numbering_game";
  char base_path[300], numbered1[300], numbered2[300];
  TEST_CHECK(save_build_path(base_path, sizeof(base_path), SAVE_STATUS_ONGOING, name));
  snprintf(numbered1, sizeof(numbered1), "games/ongoing_%s(1).chess", name);
  snprintf(numbered2, sizeof(numbered2), "games/ongoing_%s(2).chess", name);
  remove(base_path);
  remove(numbered1);
  remove(numbered2);

  char target[300];

  /* Nothing there yet: the plain name is free. */
  TEST_CHECK(save_target_path(target, sizeof(target), SAVE_STATUS_ONGOING, name, "aaaaaa"));
  TEST_CHECK(strcmp(target, base_path) == 0);

  GameState other = {0};
  position_init(&other.position);
  other.start_position = other.position;
  snprintf(other.id, sizeof(other.id), "bbbbbb");
  TEST_CHECK(save_write(base_path, &other));

  /* Taken by a different id: (1) is offered. */
  TEST_CHECK(save_target_path(target, sizeof(target), SAVE_STATUS_ONGOING, name, "aaaaaa"));
  TEST_CHECK_MSG(strcmp(target, numbered1) == 0, "expected %s, got %s", numbered1, target);

  /* But the id that already owns the plain name is recognised as its own, not
   * a collision to number past. */
  TEST_CHECK(save_target_path(target, sizeof(target), SAVE_STATUS_ONGOING, name, "bbbbbb"));
  TEST_CHECK(strcmp(target, base_path) == 0);

  GameState other2 = {0};
  position_init(&other2.position);
  other2.start_position = other2.position;
  snprintf(other2.id, sizeof(other2.id), "cccccc");
  TEST_CHECK(save_write(numbered1, &other2));

  /* (1) is now taken too: (2) is the next free number. */
  TEST_CHECK(save_target_path(target, sizeof(target), SAVE_STATUS_ONGOING, name, "aaaaaa"));
  TEST_CHECK_MSG(strcmp(target, numbered2) == 0, "expected %s, got %s", numbered2, target);

  remove(base_path);
  remove(numbered1);
  remove(numbered2);
}

static void test_no_file(void) {
  remove("test_save_does_not_exist.chess");
  GameState state = {0};
  Save_read_result_t r = save_read("test_save_does_not_exist.chess", &state);
  TEST_CHECK(r.status == SAVE_READ_NO_FILE);
}

static void test_write_then_read_matches_captures(void) {
  /* A short game with a capture, to exercise the captures list through a
   * round trip as well as the position. */
  write_raw(TMP_A, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                    "e2e4 d7d5 e4d5\n");
  GameState state = {0};
  Save_read_result_t r = save_read(TMP_A, &state);
  TEST_CHECK(r.status == SAVE_READ_OK);
  TEST_CHECK(state.p_captures_white_head != NULL);
  TEST_CHECK(state.p_captures_white_head->piece.type == PAWN);
  free_state(&state);
  remove(TMP_A);
}

void test_save(void) {
  test_round_trips();
  test_rejections();
  test_trailer();
  test_trailer_rejections();
  test_save_naming();
  test_numbering();
  test_no_file();
  test_write_then_read_matches_captures();
}
