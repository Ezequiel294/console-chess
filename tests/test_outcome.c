#include "test.h"

#include "core/notation.h"
#include "core/outcome.h"
#include "core/position.h"

static void test_checkmate(void) {
  /* White king boxed in by its own pawns, mated along the back rank. */
  Position pos;
  TEST_CHECK(fen_parse("4k3/8/8/8/8/8/5PPP/r5K1 w - - 0 1", &pos));
  Outcome_t o = outcome(&pos, NULL, 0);
  TEST_CHECK(o.reason == OUTCOME_CHECKMATE);
  TEST_CHECK(o.winner == BLACK);
}

static void test_check_with_escape(void) {
  /* In check, but the rook can be captured: not checkmate. */
  Position pos;
  TEST_CHECK(fen_parse("4k3/8/8/8/8/8/6PP/r5KR w - - 0 1", &pos));
  Outcome_t o = outcome(&pos, NULL, 0);
  TEST_CHECK(o.reason == OUTCOME_IN_PROGRESS);
}

static void test_stalemate(void) {
  /* Black king in the corner, not in check, with every flight square covered
   * by the white queen alone. */
  Position pos;
  TEST_CHECK(fen_parse("k7/8/1Q6/8/8/8/8/7K b - - 0 1", &pos));
  Outcome_t o = outcome(&pos, NULL, 0);
  TEST_CHECK(o.reason == OUTCOME_STALEMATE);
  TEST_CHECK(o.winner == NONE);
}

static void test_fifty_move(void) {
  Position pos;
  TEST_CHECK(fen_parse("7k/8/8/8/8/8/R3K3/8 w - - 100 60", &pos));
  Outcome_t o = outcome(&pos, NULL, 0);
  TEST_CHECK(o.reason == OUTCOME_DRAW_FIFTY_MOVE);

  /* One half-move short: the game continues. */
  Position not_yet;
  TEST_CHECK(fen_parse("7k/8/8/8/8/8/R3K3/8 w - - 99 60", &not_yet));
  Outcome_t o2 = outcome(&not_yet, NULL, 0);
  TEST_CHECK(o2.reason == OUTCOME_IN_PROGRESS);
}

static void test_insufficient_material(void) {
  Position bare_kings;
  TEST_CHECK(fen_parse("8/8/4k3/8/8/4K3/8/8 w - - 0 1", &bare_kings));
  TEST_CHECK(outcome(&bare_kings, NULL, 0).reason == OUTCOME_DRAW_INSUFFICIENT_MATERIAL);

  Position lone_bishop;
  TEST_CHECK(fen_parse("8/8/4k3/8/8/4KB2/8/8 w - - 0 1", &lone_bishop));
  TEST_CHECK(outcome(&lone_bishop, NULL, 0).reason == OUTCOME_DRAW_INSUFFICIENT_MATERIAL);

  Position lone_knight;
  TEST_CHECK(fen_parse("8/8/4k3/8/8/4KN2/8/8 w - - 0 1", &lone_knight));
  TEST_CHECK(outcome(&lone_knight, NULL, 0).reason == OUTCOME_DRAW_INSUFFICIENT_MATERIAL);

  /* Same-coloured bishops: c1 and f8 are both dark squares. */
  Position same_bishops;
  TEST_CHECK(fen_parse("5b1k/8/8/8/8/8/8/K1B5 w - - 0 1", &same_bishops));
  TEST_CHECK(outcome(&same_bishops, NULL, 0).reason == OUTCOME_DRAW_INSUFFICIENT_MATERIAL);

  /* A pawn, rook, or queen keeps the game going. */
  Position with_pawn;
  TEST_CHECK(fen_parse("8/8/4k3/8/8/4KP2/8/8 w - - 0 1", &with_pawn));
  TEST_CHECK(outcome(&with_pawn, NULL, 0).reason == OUTCOME_IN_PROGRESS);

  /* Two knights: not among the enumerated draws, so play continues. */
  Position two_knights;
  TEST_CHECK(fen_parse("8/8/4k3/8/8/4KN1N/8/8 w - - 0 1", &two_knights));
  TEST_CHECK(outcome(&two_knights, NULL, 0).reason == OUTCOME_IN_PROGRESS);
}

static void test_repetition(void) {
  Position pos;
  position_init(&pos);

  /* Occurred once before: two occurrences total, not yet a draw. */
  uint64_t once[1] = {pos.hash};
  TEST_CHECK(outcome(&pos, once, 1).reason == OUTCOME_IN_PROGRESS);

  /* Occurred twice before: three occurrences total, a draw. */
  uint64_t twice[2] = {pos.hash, pos.hash};
  TEST_CHECK(outcome(&pos, twice, 2).reason == OUTCOME_DRAW_REPETITION);

  /* Different castling rights make it a different position, even with an
   * identical board — so it does not count toward repetition. */
  Position moved_rights = pos;
  moved_rights.castling_rights &= ~CASTLE_WK;
  position_compute_hash(&moved_rights);
  uint64_t other[2] = {moved_rights.hash, moved_rights.hash};
  TEST_CHECK(outcome(&pos, other, 2).reason == OUTCOME_IN_PROGRESS);
}

void test_outcome(void) {
  test_checkmate();
  test_check_with_escape();
  test_stalemate();
  test_fifty_move();
  test_insufficient_material();
  test_repetition();
}
