#include "test.h"

/* FEN is covered by test_fen.c. */
#include "core/movegen.h"
#include "core/notation.h"
#include "core/outcome.h"
#include "core/position.h"

#include <string.h>

static void test_coordinate(void) {
  Position pos;
  position_init(&pos);

  Move m;
  TEST_CHECK(coord_to_move(&pos, "e2e4", &m));
  char out[COORD_MAX_LEN];
  move_to_coord(m, out);
  TEST_CHECK(strcmp(out, "e2e4") == 0);

  /* Not a legal move from the initial position. */
  TEST_CHECK(!coord_to_move(&pos, "e2e5", &m));
  /* Not even a well-formed square pair. */
  TEST_CHECK(!coord_to_move(&pos, "z9e5", &m));
  TEST_CHECK(!coord_to_move(&pos, "e2e", &m));

  /* Promotion suffix, both ways. */
  Position promo;
  TEST_CHECK(fen_parse("8/4P3/8/8/4k3/8/8/4K3 w - - 0 1", &promo));
  TEST_CHECK(coord_to_move(&promo, "e7e8n", &m));
  move_to_coord(m, out);
  TEST_CHECK(strcmp(out, "e7e8n") == 0);
  TEST_CHECK(!coord_to_move(&promo, "e7e8", &m)); /* no legal move omits the promotion suffix */

  /* Castling is written as the king's own origin and destination. */
  Position castling;
  TEST_CHECK(fen_parse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", &castling));
  TEST_CHECK(coord_to_move(&castling, "e1g1", &m));
  move_to_coord(m, out);
  TEST_CHECK(strcmp(out, "e1g1") == 0);
}

static void test_san_disambiguation(void) {
  /* Two white knights can both reach d2: b1 and f3. Neither shares the
   * other's file or rank with the third possibility removed, so file alone
   * disambiguates. */
  Position pos;
  TEST_CHECK(fen_parse("4k3/8/8/8/8/5N2/8/1N2K3 w - - 0 1", &pos));
  MoveList moves;
  generate_legal_moves(&pos, &moves);

  int found_b1 = 0, found_f3 = 0;
  for (int k = 0; k < moves.count; k++) {
    Move m = moves.moves[k];
    if (m.to_i != 6 || m.to_j != 3) { /* d2 */
      continue;
    }
    char san[SAN_MAX_LEN];
    move_to_san(&pos, m, san);
    if (m.from_i == 7 && m.from_j == 1) { /* b1 */
      TEST_CHECK_MSG(strcmp(san, "Nbd2") == 0, "got \"%s\"", san);
      found_b1 = 1;
    } else if (m.from_i == 5 && m.from_j == 5) { /* f3 */
      TEST_CHECK_MSG(strcmp(san, "Nfd2") == 0, "got \"%s\"", san);
      found_f3 = 1;
    }
  }
  TEST_CHECK(found_b1 && found_f3);

  /* Same file, different rank: rank disambiguates instead. Two rooks on the
   * a-file, both able to reach a4. */
  Position rank_case;
  TEST_CHECK(fen_parse("4k3/8/8/R7/8/8/8/R3K3 w - - 0 1", &rank_case));
  generate_legal_moves(&rank_case, &moves);
  for (int k = 0; k < moves.count; k++) {
    Move m = moves.moves[k];
    if (m.to_i != 4 || m.to_j != 0) { /* a4 */
      continue;
    }
    char san[SAN_MAX_LEN];
    move_to_san(&rank_case, m, san);
    if (m.from_i == 7) { /* a1 */
      TEST_CHECK_MSG(strcmp(san, "R1a4") == 0, "got \"%s\"", san);
    } else if (m.from_i == 3) { /* a5 */
      TEST_CHECK_MSG(strcmp(san, "R5a4") == 0, "got \"%s\"", san);
    }
  }
}

static void test_san_castling_and_promotion(void) {
  Position castling;
  TEST_CHECK(fen_parse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", &castling));
  MoveList moves;
  generate_legal_moves(&castling, &moves);
  for (int k = 0; k < moves.count; k++) {
    char san[SAN_MAX_LEN];
    move_to_san(&castling, moves.moves[k], san);
    if (moves.moves[k].flags & MOVE_CASTLE_KINGSIDE) {
      TEST_CHECK_MSG(strcmp(san, "O-O") == 0, "got \"%s\"", san);
    } else if (moves.moves[k].flags & MOVE_CASTLE_QUEENSIDE) {
      TEST_CHECK_MSG(strcmp(san, "O-O-O") == 0, "got \"%s\"", san);
    }
  }

  /* Promotion with check: a pawn promotes to a queen on e8 and gives check. */
  Position promo;
  TEST_CHECK(fen_parse("7k/4P3/8/8/8/8/8/4K3 w - - 0 1", &promo));
  generate_legal_moves(&promo, &moves);
  int seen = 0;
  for (int k = 0; k < moves.count; k++) {
    Move m = moves.moves[k];
    if (m.to_i == 0 && m.to_j == 4 && m.promotion == QUEEN) { /* e8=Q */
      char san[SAN_MAX_LEN];
      move_to_san(&promo, m, san);
      TEST_CHECK_MSG(strcmp(san, "e8=Q+") == 0, "got \"%s\"", san);
      seen = 1;
    }
  }
  TEST_CHECK(seen);
}

/* Fool's Mate: the shortest possible game, and unambiguous about every SAN
 * feature it touches — a plain advance, a capture-free development move, and
 * a queen move that delivers checkmate on the diagonal opened by White's own
 * pawns. */
static void test_san_recorded_game(void) {
  Position pos;
  position_init(&pos);

  static const char *coords[] = {"f2f3", "e7e5", "g2g4", "d8h4"};
  static const char *expected[] = {"f3", "e5", "g4", "Qh4#"};

  for (int i = 0; i < 4; i++) {
    Move m;
    TEST_CHECK_MSG(coord_to_move(&pos, coords[i], &m), "move %d (%s) is not legal", i,
                   coords[i]);
    char san[SAN_MAX_LEN];
    move_to_san(&pos, m, san);
    TEST_CHECK_MSG(strcmp(san, expected[i]) == 0, "move %d: got \"%s\", expected \"%s\"", i,
                   san, expected[i]);
    make(&pos, m);
  }

  Outcome_t o = outcome(&pos, NULL, 0);
  TEST_CHECK(o.reason == OUTCOME_CHECKMATE);
  TEST_CHECK(o.winner == BLACK);
}

void test_notation(void) {
  test_coordinate();
  test_san_disambiguation();
  test_san_castling_and_promotion();
  test_san_recorded_game();
}
