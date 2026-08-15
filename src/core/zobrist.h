#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "types.h"

/* Zobrist keys for incremental position hashing.
 *
 * Position.hash is XORed with these keys as pieces move, rather than
 * recomputed from the board on every make/unmake, so that comparing two
 * positions for threefold repetition is a 64-bit integer comparison instead
 * of an O(n) struct comparison repeated O(n) times.
 *
 * Keys are generated from a fixed seed, not rand(), so the same position
 * hashes the same way on every run — which matters for tests and for the
 * fixed reference behaviour perft depends on.
 */

/* Populates the key tables. Idempotent; safe to call more than once. Called
 * lazily by the functions below, so nothing else needs to call it. */
void zobrist_init(void);

uint64_t zobrist_piece_key(Color color, Piece_type_t type, int i, int j);
uint64_t zobrist_side_key(void);
/* bit is one of the CASTLE_* constants from types.h. */
uint64_t zobrist_castle_key(unsigned bit);
uint64_t zobrist_ep_file_key(int file);

/* Computes a position's hash from scratch: piece placement, side to move,
 * castling rights, and the en passant file (not rank — the rank is implied by
 * whose move it is, exactly as FEN and the repetition rule treat it). Used to
 * seed Position.hash on construction; make()/unmake() maintain it
 * incrementally from there. */
uint64_t zobrist_hash(const Position *pos);

#endif /* ZOBRIST_H */
