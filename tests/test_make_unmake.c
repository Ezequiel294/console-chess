#include "test.h"

#include "core/movegen.h"
#include "core/notation.h"

#include <string.h>

static void check_position(const char *fen) {
  Position original;
  TEST_CHECK_MSG(fen_parse(fen, &original), "bad fixture FEN: %s", fen);

  MoveList moves;
  generate_legal_moves(&original, &moves);
  TEST_CHECK_MSG(moves.count > 0, "no legal moves from fixture: %s", fen);

  for (int k = 0; k < moves.count; k++) {
    Position pos = original;
    make(&pos, moves.moves[k]);
    unmake(&pos, moves.moves[k]);
    TEST_CHECK_MSG(memcmp(&pos, &original, sizeof(Position)) == 0,
                    "make/unmake did not restore %s move %d (from %d,%d to %d,%d)", fen, k,
                    moves.moves[k].from_i, moves.moves[k].from_j, moves.moves[k].to_i,
                    moves.moves[k].to_j);
  }
}

void test_make_unmake(void) {
  check_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  check_position("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
  check_position("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b KQkq - 0 1");
  check_position("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
  check_position("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
  check_position("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
  check_position("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 b - - 0 1");
}
