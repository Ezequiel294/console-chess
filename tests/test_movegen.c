#include "test.h"

#include "core/board.h"
#include "core/movegen.h"
#include "core/notation.h"
#include "core/position.h"

void test_movegen(void) {
  /* The starting position has exactly 20 legal moves: 16 pawn moves (two
   * squares each for eight pawns) and 4 knight moves. */
  Position init;
  position_init(&init);
  MoveList moves;
  generate_legal_moves(&init, &moves);
  TEST_CHECK_MSG(moves.count == 20, "expected 20 moves from the initial position, got %d",
                 moves.count);

  /* Own pieces are never a legal destination. */
  for (int k = 0; k < moves.count; k++) {
    Move m = moves.moves[k];
    TEST_CHECK(init.board[m.to_i][m.to_j].color != WHITE);
  }

  /* A bishop pinned to its king along a diagonal may only move within the
   * pin: white king e1, white bishop d2, black bishop a5 pinning it. */
  Position pin;
  TEST_CHECK(fen_parse("4k3/8/8/b7/8/8/3B4/4K3 w - - 0 1", &pin));
  int bi, bj;
  square_to_index("d2", &bi, &bj);
  MoveList bishop_moves;
  generate_legal_moves_from(&pin, bi, bj, &bishop_moves);
  for (int k = 0; k < bishop_moves.count; k++) {
    Move m = bishop_moves.moves[k];
    /* Every legal destination stays on the a5-e1 diagonal (i - j == 3), even
     * though the bishop's unconstrained geometry would also allow the other
     * diagonal through d2. */
    TEST_CHECK(m.to_i - m.to_j == 3);
  }
  /* b4, c3, and capturing on a5: exactly the pin diagonal, no more. */
  TEST_CHECK_MSG(bishop_moves.count == 3, "expected 3 moves along the pin, got %d",
                 bishop_moves.count);

  /* The same pin removes every move of a piece that is not the bishop but
   * would otherwise be free — here, nothing else needs to move, so instead
   * verify a pinned rook (not on the diagonal) simply has none: white king
   * e1, white rook e2, black rook e8 pinning along the e-file. */
  Position pin2;
  TEST_CHECK(fen_parse("4r2k/8/8/8/8/8/4R3/4K3 w - - 0 1", &pin2));
  int ri, rj;
  square_to_index("e2", &ri, &rj);
  MoveList rook_moves;
  generate_legal_moves_from(&pin2, ri, rj, &rook_moves);
  for (int k = 0; k < rook_moves.count; k++) {
    /* Confined to the e-file, the only line that keeps the king shielded. */
    TEST_CHECK(rook_moves.moves[k].to_j == rj);
  }
  /* e3 through e7, plus capturing the pinning rook on e8. */
  TEST_CHECK_MSG(rook_moves.count == 6, "expected 6 moves along the pin, got %d",
                 rook_moves.count);

  /* A king never has a legal move onto an attacked square: white king e1,
   * black rook on d5 controlling the whole d-file from a distance, so d1 and
   * d2 are attacked (and empty — the rook itself is not the destination,
   * which would legally be capturable) while e2, f1, and f2 are not. */
  Position king_pos;
  TEST_CHECK(fen_parse("7k/8/8/3r4/8/8/8/4K3 w - - 0 1", &king_pos));
  int ki, kj;
  square_to_index("e1", &ki, &kj);
  MoveList king_moves;
  generate_legal_moves_from(&king_pos, ki, kj, &king_moves);
  for (int k = 0; k < king_moves.count; k++) {
    Move m = king_moves.moves[k];
    TEST_CHECK(m.to_j != 3); /* the d-file is covered by the rook */
  }
  TEST_CHECK_MSG(king_moves.count == 3, "expected 3 legal king moves, got %d",
                 king_moves.count);

  /* Response to check is forced: every generated move leaves the mover out of
   * check. White king in check from a black rook on the e-file, with a knight
   * that can block. */
  Position check_pos;
  TEST_CHECK(fen_parse("4r2k/8/8/3N4/8/8/8/4K3 w - - 0 1", &check_pos));
  TEST_CHECK(in_check(&check_pos, WHITE));
  MoveList responses;
  generate_legal_moves(&check_pos, &responses);
  for (int k = 0; k < responses.count; k++) {
    Position scratch = check_pos;
    make(&scratch, responses.moves[k]);
    TEST_CHECK(!in_check(&scratch, WHITE));
  }
  TEST_CHECK(responses.count > 0);
}
