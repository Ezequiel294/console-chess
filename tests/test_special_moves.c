#include "test.h"

#include "core/board.h"
#include "core/movegen.h"
#include "core/notation.h"

static int has_castle(const MoveList *moves, unsigned flag) {
  for (int k = 0; k < moves->count; k++) {
    if (moves->moves[k].flags & flag) {
      return 1;
    }
  }
  return 0;
}

static int has_destination(const MoveList *moves, int to_i, int to_j) {
  for (int k = 0; k < moves->count; k++) {
    if (moves->moves[k].to_i == to_i && moves->moves[k].to_j == to_j) {
      return 1;
    }
  }
  return 0;
}

static void test_castling(void) {
  /* Rights intact, path clear, nothing attacked: both wings available. */
  Position clear;
  TEST_CHECK(fen_parse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", &clear));
  int ki, kj;
  square_to_index("e1", &ki, &kj);
  MoveList moves;
  generate_legal_moves_from(&clear, ki, kj, &moves);
  TEST_CHECK(has_castle(&moves, MOVE_CASTLE_KINGSIDE));
  TEST_CHECK(has_castle(&moves, MOVE_CASTLE_QUEENSIDE));

  /* In check: no castling on either wing. */
  Position in_check_pos;
  TEST_CHECK(fen_parse("4r2k/8/8/8/8/8/8/R3K2R w KQ - 0 1", &in_check_pos));
  generate_legal_moves_from(&in_check_pos, ki, kj, &moves);
  TEST_CHECK(!has_castle(&moves, MOVE_CASTLE_KINGSIDE));
  TEST_CHECK(!has_castle(&moves, MOVE_CASTLE_QUEENSIDE));

  /* The square the king crosses (f1) is attacked: kingside is withheld even
   * though the king's own square and destination are safe. */
  Position through_attacked;
  TEST_CHECK(fen_parse("5r1k/8/8/8/8/8/8/R3K2R w KQ - 0 1", &through_attacked));
  generate_legal_moves_from(&through_attacked, ki, kj, &moves);
  TEST_CHECK(!has_castle(&moves, MOVE_CASTLE_KINGSIDE));
  TEST_CHECK(has_castle(&moves, MOVE_CASTLE_QUEENSIDE));

  /* The rook's own square is attacked, but the king's origin, path, and
   * destination are not: castling is still legal. */
  Position rook_attacked;
  TEST_CHECK(fen_parse("6kr/8/8/8/8/8/8/R3K2R w KQ - 0 1", &rook_attacked));
  generate_legal_moves_from(&rook_attacked, ki, kj, &moves);
  TEST_CHECK(has_castle(&moves, MOVE_CASTLE_KINGSIDE));

  /* Rights lost by moving the king, even after it returns to e1. */
  Position king_walk;
  TEST_CHECK(fen_parse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", &king_walk));
  MoveList step;
  generate_legal_moves_from(&king_walk, ki, kj, &step);
  int e2i, e2j;
  square_to_index("e2", &e2i, &e2j);
  for (int k = 0; k < step.count; k++) {
    if (step.moves[k].to_i == e2i && step.moves[k].to_j == e2j) {
      make(&king_walk, step.moves[k]);
      break;
    }
  }
  TEST_CHECK((king_walk.castling_rights & (CASTLE_WK | CASTLE_WQ)) == 0);
  TEST_CHECK(king_walk.side_to_move == BLACK);

  /* Black must move before white can move again. */
  int bki, bkj;
  square_to_index("e8", &bki, &bkj);
  MoveList black_step;
  generate_legal_moves_from(&king_walk, bki, bkj, &black_step);
  TEST_CHECK(black_step.count > 0);
  make(&king_walk, black_step.moves[0]);
  TEST_CHECK(king_walk.side_to_move == WHITE);

  generate_legal_moves_from(&king_walk, e2i, e2j, &step);
  for (int k = 0; k < step.count; k++) {
    if (step.moves[k].to_i == ki && step.moves[k].to_j == kj) {
      make(&king_walk, step.moves[k]);
      break;
    }
  }
  TEST_CHECK(king_walk.side_to_move == BLACK);

  /* Black moves again, handing the turn back to white, so white's moves from
   * e1 can be queried with the king home and both rooks untouched. */
  generate_legal_moves(&king_walk, &black_step);
  TEST_CHECK(black_step.count > 0);
  make(&king_walk, black_step.moves[0]);
  TEST_CHECK(king_walk.side_to_move == WHITE);

  generate_legal_moves_from(&king_walk, ki, kj, &moves);
  TEST_CHECK(!has_castle(&moves, MOVE_CASTLE_KINGSIDE));
  TEST_CHECK(!has_castle(&moves, MOVE_CASTLE_QUEENSIDE));

  /* Rights lost when a rook is captured on its starting square. */
  Position rook_capture;
  TEST_CHECK(fen_parse("4k2q/8/8/8/8/8/8/R3K2R b KQ - 0 1", &rook_capture));
  int h1i, h1j;
  square_to_index("h1", &h1i, &h1j);
  MoveList queen_moves;
  square_to_index("h8", &ki, &kj);
  generate_legal_moves_from(&rook_capture, ki, kj, &queen_moves);
  int taken = 0;
  for (int k = 0; k < queen_moves.count; k++) {
    if (queen_moves.moves[k].to_i == h1i && queen_moves.moves[k].to_j == h1j) {
      make(&rook_capture, queen_moves.moves[k]);
      taken = 1;
      break;
    }
  }
  TEST_CHECK(taken);
  TEST_CHECK((rook_capture.castling_rights & CASTLE_WK) == 0);
  TEST_CHECK((rook_capture.castling_rights & CASTLE_WQ) != 0);
}

static void test_en_passant(void) {
  /* Capture available the move immediately after a double advance, and the
   * captured pawn is removed from its own square, not the destination. */
  Position pos;
  TEST_CHECK(fen_parse("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", &pos));
  int fi, fj;
  square_to_index("e5", &fi, &fj);
  MoveList moves;
  generate_legal_moves_from(&pos, fi, fj, &moves);
  int di, dj;
  square_to_index("d6", &di, &dj);
  TEST_CHECK(has_destination(&moves, di, dj));

  for (int k = 0; k < moves.count; k++) {
    if (moves.moves[k].to_i == di && moves.moves[k].to_j == dj) {
      Position after = pos;
      make(&after, moves.moves[k]);
      int cap_i, cap_j;
      square_to_index("d5", &cap_i, &cap_j);
      TEST_CHECK(after.board[cap_i][cap_j].type == FREE);
      TEST_CHECK(after.board[di][dj].type == PAWN && after.board[di][dj].color == WHITE);
      break;
    }
  }

  /* The opportunity expires: any other move clears it, and it is not
   * available on the following turn. */
  Position expired;
  TEST_CHECK(fen_parse("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", &expired));
  int e1i, e1j;
  square_to_index("e1", &e1i, &e1j);
  MoveList king_moves;
  generate_legal_moves_from(&expired, e1i, e1j, &king_moves);
  TEST_CHECK(king_moves.count > 0);
  make(&expired, king_moves.moves[0]); /* white plays something else */
  TEST_CHECK(expired.ep_i < 0);

  /* The pin along the rank: capturing en passant would remove both the
   * capturing and captured pawn from the rank, exposing the king to a rook
   * beyond. The capture must not be generated. */
  Position pin;
  TEST_CHECK(fen_parse("4k3/8/8/K3Pp1r/8/8/8/8 w - f6 0 1", &pin));
  int pi, pj;
  square_to_index("e5", &pi, &pj);
  MoveList pin_moves;
  generate_legal_moves_from(&pin, pi, pj, &pin_moves);
  int f6i, f6j;
  square_to_index("f6", &f6i, &f6j);
  TEST_CHECK(!has_destination(&pin_moves, f6i, f6j));
}

static void test_promotion(void) {
  Position pos;
  TEST_CHECK(fen_parse("8/4P3/8/8/4k3/8/8/4K3 w - - 0 1", &pos));
  int fi, fj;
  square_to_index("e7", &fi, &fj);
  MoveList moves;
  generate_legal_moves_from(&pos, fi, fj, &moves);

  int ti, tj;
  square_to_index("e8", &ti, &tj);
  int seen[5] = {0}; /* indexed by Piece_type_t: QUEEN, ROOK, KNIGHT, BISHOP */
  int count_to_e8 = 0;
  for (int k = 0; k < moves.count; k++) {
    if (moves.moves[k].to_i == ti && moves.moves[k].to_j == tj) {
      count_to_e8++;
      seen[moves.moves[k].promotion] = 1;
    }
  }
  TEST_CHECK_MSG(count_to_e8 == 4, "expected 4 promotion choices, got %d", count_to_e8);
  TEST_CHECK(seen[QUEEN] && seen[ROOK] && seen[BISHOP] && seen[KNIGHT]);

  /* A pawn never remains a pawn on the far rank. */
  for (int k = 0; k < moves.count; k++) {
    if (moves.moves[k].to_i == ti && moves.moves[k].to_j == tj) {
      TEST_CHECK(moves.moves[k].promotion != FREE);
    }
  }

  /* Promotion by capture also yields four choices. */
  Position capture;
  TEST_CHECK(fen_parse("3n4/4P3/8/8/4k3/8/8/4K3 w - - 0 1", &capture));
  generate_legal_moves_from(&capture, fi, fj, &moves);
  int d8i, d8j;
  square_to_index("d8", &d8i, &d8j);
  int count_to_d8 = 0;
  for (int k = 0; k < moves.count; k++) {
    if (moves.moves[k].to_i == d8i && moves.moves[k].to_j == d8j) {
      count_to_d8++;
    }
  }
  TEST_CHECK_MSG(count_to_d8 == 4, "expected 4 promotion-by-capture choices, got %d",
                 count_to_d8);
}

void test_special_moves(void) {
  test_castling();
  test_en_passant();
  test_promotion();
}
