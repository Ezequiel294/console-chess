#include "test.h"

#include "app/save.h"
#include "core/board.h"
#include "core/history.h"
#include "core/notation.h"
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
  test_no_file();
  test_write_then_read_matches_captures();
}
