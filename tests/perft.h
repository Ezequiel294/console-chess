#ifndef PERFT_H
#define PERFT_H

#include "core/movegen.h"
#include "core/position.h"

/* Counts the leaf nodes of the move tree rooted at *pos, to the given depth.
 * The standard correctness test for a move generator: from a known position
 * the answer is a fixed number, and matching it proves castling, en passant,
 * promotion, and pin handling simultaneously. *pos is restored to its
 * original state before returning (every move made is unmade). */
long long perft(Position *pos, int depth);

/* One root move and the leaf count of the subtree beneath it. */
typedef struct {
  Move move;
  long long count;
} PerftDivide;

/* Divided perft: perft(depth - 1) computed separately for each legal root
 * move, so a mismatch against a reference count can be bisected to the
 * offending move in a few steps rather than searched for by hand. Returns the
 * number of entries written to out, which must have room for at least
 * MAX_MOVES. */
int perft_divide(Position *pos, int depth, PerftDivide *out);

#endif /* PERFT_H */
