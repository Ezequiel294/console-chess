#include "test.h"

#include "core/notation.h"
#include "core/position.h"

#include <string.h>

static void check_round_trip(const char *fen) {
  Position pos;
  int ok = fen_parse(fen, &pos);
  TEST_CHECK_MSG(ok, "fen_parse rejected a well-formed FEN: %s", fen);
  if (!ok) {
    return;
  }

  char out[FEN_MAX_LEN];
  fen_write(&pos, out);
  TEST_CHECK_MSG(strcmp(fen, out) == 0, "round trip mismatch: %s -> %s", fen, out);

  /* Reading the written text back must reproduce the same position again. */
  Position pos2;
  TEST_CHECK(fen_parse(out, &pos2));
  TEST_CHECK(memcmp(pos.board, pos2.board, sizeof(pos.board)) == 0);
  TEST_CHECK(pos.side_to_move == pos2.side_to_move);
  TEST_CHECK(pos.castling_rights == pos2.castling_rights);
  TEST_CHECK(pos.ep_i == pos2.ep_i && pos.ep_j == pos2.ep_j);
  TEST_CHECK(pos.halfmove_clock == pos2.halfmove_clock);
  TEST_CHECK(pos.fullmove_number == pos2.fullmove_number);
}

static void check_rejected(const char *fen) {
  Position pos;
  memset(&pos, 0xAB, sizeof(pos)); /* poisoned, to catch a partial write */
  Position before = pos;
  int ok = fen_parse(fen, &pos);
  TEST_CHECK_MSG(!ok, "fen_parse accepted malformed input: %s", fen);
  TEST_CHECK_MSG(memcmp(&pos, &before, sizeof(pos)) == 0,
                 "fen_parse wrote a partial position for: %s", fen);
}

void test_fen(void) {
  check_round_trip("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  check_round_trip("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
  check_round_trip("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
  check_round_trip("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
  check_round_trip("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
  check_round_trip("4k3/8/8/8/8/8/8/4K2R w K - 0 1");
  check_round_trip("r3k2r/8/8/8/8/8/8/4K3 b kq - 3 20");

  /* Standard initial position writes to the well-known string. */
  Position init;
  position_init(&init);
  char out[FEN_MAX_LEN];
  fen_write(&init, out);
  TEST_CHECK(strcmp(out, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") == 0);

  /* Malformed input. */
  check_rejected("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0"); /* 5 fields */
  check_rejected("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 extra"); /* 7 fields */
  check_rejected("rnbqkbnr/ppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); /* rank too short */
  check_rejected("rnbqkbnr/ppppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); /* rank too long */
  check_rejected("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKXNR w KQkq - 0 1"); /* unknown letter */
  check_rejected("rnbq1bnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQ1BNR w KQkq - 0 1"); /* no kings */
  check_rejected("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQ1BNR w KQkq - 0 1"); /* missing white king */
  check_rejected("");
}
