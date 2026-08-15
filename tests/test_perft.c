#include "test.h"

#include "core/board.h"
#include "core/notation.h"
#include "perft.h"

#include <stdio.h>
#include <string.h>

/* On a mismatch, divided perft bisects to the offending root move instead of
 * leaving a single wrong number to stare at. */
static void report_mismatch(Position *pos, int depth, long long expected, long long got) {
  fprintf(stderr, "  perft(%d) = %lld, expected %lld — dividing:\n", depth, got, expected);
  PerftDivide divide[MAX_MOVES];
  int n = perft_divide(pos, depth, divide);
  for (int i = 0; i < n; i++) {
    char from[3], to[3];
    index_to_square(divide[i].move.from_i, divide[i].move.from_j, from);
    index_to_square(divide[i].move.to_i, divide[i].move.to_j, to);
    fprintf(stderr, "    %s%s: %lld\n", from, to, divide[i].count);
  }
}

static void check_perft(const char *fen, int depth, long long expected) {
  Position pos;
  int parsed = fen_parse(fen, &pos);
  TEST_CHECK_MSG(parsed, "bad fixture FEN: %s", fen);
  if (!parsed) {
    return;
  }
  Position before = pos;
  long long got = perft(&pos, depth);
  TEST_CHECK_MSG(got == expected, "perft(\"%s\", %d) = %lld, expected %lld", fen, depth, got,
                 expected);
  if (got != expected) {
    report_mismatch(&pos, depth, expected, got);
  }
  /* perft must leave the position exactly as it found it. */
  TEST_CHECK(memcmp(&pos, &before, sizeof(Position)) == 0);
}

void test_perft(void) {
  const char *initial = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  const char *kiwipete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
  /* The classic en passant pin position. */
  const char *ep_pin = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
  /* The classic promotion-with-check position. */
  const char *promotion = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";

  check_perft(initial, 1, 20);
  check_perft(initial, 2, 400);
  check_perft(initial, 3, 8902);
  check_perft(initial, 4, 197281);

  check_perft(kiwipete, 1, 48);
  check_perft(kiwipete, 2, 2039);
  check_perft(kiwipete, 3, 97862);

  check_perft(ep_pin, 1, 14);
  check_perft(ep_pin, 2, 191);
  check_perft(ep_pin, 3, 2812);

  check_perft(promotion, 1, 44);
  check_perft(promotion, 2, 1486);
  check_perft(promotion, 3, 62379);

  if (!g_test_full) {
    return;
  }

  check_perft(initial, 5, 4865609);
  check_perft(kiwipete, 4, 4085603);
  check_perft(ep_pin, 4, 43238);
  check_perft(ep_pin, 5, 674624);
  check_perft(promotion, 4, 2103487);
}
