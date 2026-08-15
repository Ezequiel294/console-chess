#include "core/outcome.h"

#include "core/movegen.h"
#include "core/position.h"

/* King against king, king and a minor against king, or king and bishop
 * against king and bishop with both bishops on the same colour of square —
 * the only material configurations that can never force checkmate. Anything
 * else, including two knights, is left as sufficient: the spec draws the
 * line at these four cases and no further. */
static int insufficient_material(const Position *pos) {
  int minors[2] = {0, 0}; /* indexed by Color: WHITE, BLACK */
  Piece_type_t minor_type[2] = {FREE, FREE};
  int bishop_light[2] = {-1, -1};

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      Piece_t p = pos->board[i][j];
      if (p.type == FREE || p.type == KING) {
        continue;
      }
      if (p.type == PAWN || p.type == ROOK || p.type == QUEEN) {
        return 0;
      }
      minors[p.color]++;
      minor_type[p.color] = p.type;
      if (p.type == BISHOP) {
        bishop_light[p.color] = square_is_light(i, j);
      }
    }
  }

  if (minors[WHITE] == 0 && minors[BLACK] == 0) {
    return 1; /* bare kings */
  }
  if ((minors[WHITE] == 1 && minors[BLACK] == 0) ||
      (minors[BLACK] == 1 && minors[WHITE] == 0)) {
    return 1; /* a lone bishop or knight against a bare king */
  }
  if (minors[WHITE] == 1 && minors[BLACK] == 1 && minor_type[WHITE] == BISHOP &&
      minor_type[BLACK] == BISHOP && bishop_light[WHITE] == bishop_light[BLACK]) {
    return 1;
  }
  return 0;
}

static int repetition_count(const Position *pos, const uint64_t *hash_history, int len) {
  int count = 1; /* pos itself */
  for (int i = 0; i < len; i++) {
    if (hash_history[i] == pos->hash) {
      count++;
    }
  }
  return count;
}

Outcome_t outcome(const Position *pos, const uint64_t *hash_history, int hash_history_len) {
  MoveList moves;
  generate_legal_moves(pos, &moves);

  if (moves.count == 0) {
    if (in_check(pos, pos->side_to_move)) {
      Color winner = (pos->side_to_move == WHITE) ? BLACK : WHITE;
      return (Outcome_t){.reason = OUTCOME_CHECKMATE, .winner = winner};
    }
    return (Outcome_t){.reason = OUTCOME_STALEMATE, .winner = NONE};
  }

  if (insufficient_material(pos)) {
    return (Outcome_t){.reason = OUTCOME_DRAW_INSUFFICIENT_MATERIAL, .winner = NONE};
  }

  if (pos->halfmove_clock >= 100) {
    return (Outcome_t){.reason = OUTCOME_DRAW_FIFTY_MOVE, .winner = NONE};
  }

  if (repetition_count(pos, hash_history, hash_history_len) >= 3) {
    return (Outcome_t){.reason = OUTCOME_DRAW_REPETITION, .winner = NONE};
  }

  return (Outcome_t){.reason = OUTCOME_IN_PROGRESS, .winner = NONE};
}
