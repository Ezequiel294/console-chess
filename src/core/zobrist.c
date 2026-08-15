#include "core/zobrist.h"

/* Indexed [color][type][i][j]. FREE has no key: an empty square contributes
 * nothing to the hash, which is what lets pieces be XORed out on departure and
 * back in on arrival without a special case for the square they leave empty. */
static uint64_t g_piece_keys[2][6][8][8];
static uint64_t g_side_key;
static uint64_t g_castle_keys[4]; /* indexed by the bit position, not the mask */
static uint64_t g_ep_file_keys[8];
static int g_initialised = 0;

/* splitmix64, seeded fixed. Not used anywhere state needs to be unpredictable
 * — only reproducible across runs, which a fixed seed guarantees and rand()
 * does not. */
static uint64_t splitmix64_next(uint64_t *state) {
  uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

void zobrist_init(void) {
  if (g_initialised) {
    return;
  }
  g_initialised = 1;

  uint64_t state = 0xC0FFEE123456789AULL;

  for (int color = 0; color < 2; color++) {
    for (int type = 0; type < 6; type++) {
      for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
          g_piece_keys[color][type][i][j] = splitmix64_next(&state);
        }
      }
    }
  }
  g_side_key = splitmix64_next(&state);
  for (int b = 0; b < 4; b++) {
    g_castle_keys[b] = splitmix64_next(&state);
  }
  for (int f = 0; f < 8; f++) {
    g_ep_file_keys[f] = splitmix64_next(&state);
  }
}

uint64_t zobrist_piece_key(Color color, Piece_type_t type, int i, int j) {
  zobrist_init();
  return g_piece_keys[color][type][i][j];
}

uint64_t zobrist_side_key(void) {
  zobrist_init();
  return g_side_key;
}

uint64_t zobrist_castle_key(unsigned bit) {
  zobrist_init();
  int index = 0;
  while (bit > 1u) {
    bit >>= 1;
    index++;
  }
  return g_castle_keys[index];
}

uint64_t zobrist_ep_file_key(int file) {
  zobrist_init();
  return g_ep_file_keys[file];
}

uint64_t zobrist_hash(const Position *pos) {
  zobrist_init();
  uint64_t h = 0;

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      Piece_t p = pos->board[i][j];
      if (p.type != FREE) {
        h ^= zobrist_piece_key(p.color, p.type, i, j);
      }
    }
  }

  if (pos->side_to_move == BLACK) {
    h ^= g_side_key;
  }

  if (pos->castling_rights & CASTLE_WK) h ^= zobrist_castle_key(CASTLE_WK);
  if (pos->castling_rights & CASTLE_WQ) h ^= zobrist_castle_key(CASTLE_WQ);
  if (pos->castling_rights & CASTLE_BK) h ^= zobrist_castle_key(CASTLE_BK);
  if (pos->castling_rights & CASTLE_BQ) h ^= zobrist_castle_key(CASTLE_BQ);

  if (pos->ep_j >= 0) {
    h ^= zobrist_ep_file_key(pos->ep_j);
  }

  return h;
}
