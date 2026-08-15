#ifndef OUTCOME_H
#define OUTCOME_H

#include "types.h"

/* When a game has ended and why — checkmate, stalemate, or a draw — in place
 * of the previous win condition, king capture, which this layer's legality
 * filter makes unreachable: no legal move can ever leave a king attacked, so
 * a king can no longer be captured at all.
 */

typedef enum {
  OUTCOME_IN_PROGRESS,
  OUTCOME_CHECKMATE,
  OUTCOME_STALEMATE,
  OUTCOME_DRAW_FIFTY_MOVE,
  OUTCOME_DRAW_INSUFFICIENT_MATERIAL,
  OUTCOME_DRAW_REPETITION
} Outcome_reason_t;

typedef struct {
  Outcome_reason_t reason;
  Color winner; /* NONE for a draw or a game still in progress */
} Outcome_t;

/* Determines whether *pos concludes the game and why.
 *
 * hash_history is the Zobrist hash of every position reached so far in the
 * game, in order, NOT including *pos itself — outcome() accounts for the
 * current position on top of whatever matches it finds there, so a game
 * where the current position has occurred twice before is passed a history
 * containing those two hashes, and outcome() treats that as the third
 * occurrence. Pass hash_history_len 0 (any pointer, including NULL) for a
 * position with no history yet.
 */
Outcome_t outcome(const Position *pos, const uint64_t *hash_history, int hash_history_len);

#endif /* OUTCOME_H */
