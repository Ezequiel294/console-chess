#include "test.h"

#include "core/board.h"
#include "core/movegen.h"
#include "core/notation.h"

static int attacked(const char *fen, const char *square, Color by_side) {
  Position pos;
  TEST_CHECK_MSG(fen_parse(fen, &pos), "bad fixture FEN: %s", fen);
  int i, j;
  TEST_CHECK(square_to_index(square, &i, &j));
  return is_square_attacked(&pos, i, j, by_side);
}

void test_attacks(void) {
  /* Every fixture below carries both kings, in corners chosen to stay clear
   * of the lines and squares each case exercises, since fen_parse rejects a
   * position missing either one.
   *
   * Rook along an open rank. */
  TEST_CHECK(attacked("7k/8/8/8/8/8/8/R3K3 w - - 0 1", "d1", WHITE));
  /* Blocked by a piece in between: the rook no longer reaches past it. The
   * king sits on g1, clear of d1, so d1's only possible attacker is the rook. */
  TEST_CHECK(!attacked("7k/8/8/8/8/8/8/R1B3K1 w - - 0 1", "d1", WHITE));
  /* The blocking piece itself is still attacked. */
  TEST_CHECK(attacked("7k/8/8/8/8/8/8/R1B1K3 w - - 0 1", "c1", WHITE));

  /* Bishop along a diagonal, and blocked. The blocker is a black pawn rather
   * than white, so it does not itself attack h7 by pawn geometry and confound
   * the result. */
  TEST_CHECK(attacked("k7/8/8/8/4B3/8/8/K7 w - - 0 1", "g6", WHITE));
  TEST_CHECK(!attacked("k7/8/6p1/8/4B3/8/8/K7 w - - 0 1", "h7", WHITE));

  /* Queen combines both lines. Corners a1/h8 avoid both its file, its rank,
   * and both diagonals through e4. */
  TEST_CHECK(attacked("7k/8/8/8/4Q3/8/8/K7 w - - 0 1", "e8", WHITE));
  TEST_CHECK(attacked("7k/8/8/8/4Q3/8/8/K7 w - - 0 1", "a4", WHITE));
  TEST_CHECK(attacked("7k/8/8/8/4Q3/8/8/K7 w - - 0 1", "h1", WHITE));

  /* Knight, from every one of its offsets, is not blockable. */
  TEST_CHECK(attacked("k7/8/8/3p4/8/2N5/8/K7 w - - 0 1", "d5", WHITE));
  TEST_CHECK(attacked("3nk3/8/2P5/8/8/8/8/K7 b - - 0 1", "c6", BLACK));

  /* Pawns attack diagonally forward only, never straight ahead. */
  TEST_CHECK(attacked("k7/8/8/8/3p4/8/8/K7 b - - 0 1", "c3", BLACK));
  TEST_CHECK(attacked("k7/8/8/8/3p4/8/8/K7 b - - 0 1", "e3", BLACK));
  TEST_CHECK(!attacked("k7/8/8/8/3p4/8/8/K7 b - - 0 1", "d3", BLACK));
  TEST_CHECK(attacked("k7/8/8/3P4/8/8/8/K7 w - - 0 1", "c6", WHITE));

  /* King, adjacent squares only. */
  TEST_CHECK(attacked("7k/8/8/4K3/8/8/8/8 w - - 0 1", "f6", WHITE));
  TEST_CHECK(!attacked("7k/8/8/4K3/8/8/8/8 w - - 0 1", "g7", WHITE));

  /* in_check built on the same primitive. */
  Position check_pos;
  TEST_CHECK(fen_parse("4k3/8/8/8/8/8/4Q3/4K3 b - - 0 1", &check_pos));
  TEST_CHECK(in_check(&check_pos, BLACK));
  TEST_CHECK(!in_check(&check_pos, WHITE));

  Position no_check;
  TEST_CHECK(fen_parse("4k3/8/8/8/8/8/8/4K3 w - - 0 1", &no_check));
  TEST_CHECK(!in_check(&no_check, WHITE));
  TEST_CHECK(!in_check(&no_check, BLACK));
}
