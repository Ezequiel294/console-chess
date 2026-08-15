#include "perft.h"

long long perft(Position *pos, int depth) {
  if (depth == 0) {
    return 1;
  }

  MoveList moves;
  generate_legal_moves(pos, &moves);

  if (depth == 1) {
    return moves.count;
  }

  long long total = 0;
  for (int i = 0; i < moves.count; i++) {
    make(pos, moves.moves[i]);
    total += perft(pos, depth - 1);
    unmake(pos, moves.moves[i]);
  }
  return total;
}

int perft_divide(Position *pos, int depth, PerftDivide *out) {
  MoveList moves;
  generate_legal_moves(pos, &moves);

  for (int i = 0; i < moves.count; i++) {
    make(pos, moves.moves[i]);
    out[i].move = moves.moves[i];
    out[i].count = (depth > 1) ? perft(pos, depth - 1) : 1;
    unmake(pos, moves.moves[i]);
  }
  return moves.count;
}
