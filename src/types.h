#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

/* Shared data types.
 *
 * This is the only header any other header is allowed to include. Every module
 * header includes types.h and nothing else, which keeps the include graph a
 * star rather than a web and makes header cycles impossible.
 */

typedef enum { WHITE, BLACK, NONE } Color;
typedef enum { PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING, FREE } Piece_type_t;

/* A square's contents.
 *
 * Deliberately carries no presentation and no self-knowledge of where it sits.
 * The glyph is a function of (type, color) and lives in display.c; the square
 * name is a function of the indices and lives in board.c. Storing either here
 * would put a copy of derived data next to the data it derives from.
 */
typedef struct {
  Color color;
  Piece_type_t type;
} Piece_t;

/* Castling rights, one bit per side and wing. */
enum {
  CASTLE_WK = 1 << 0,
  CASTLE_WQ = 1 << 1,
  CASTLE_BK = 1 << 2,
  CASTLE_BQ = 1 << 3,
  CASTLE_ALL = CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ
};

/* A complete chess position: everything legality depends on, not just piece
 * placement. Castling and en passant are history-dependent, so two positions
 * with identical boards can permit different moves.
 *
 * Kept small enough to copy by value — the legality filter in movegen.c copies
 * it once per candidate move — so it carries no glyphs and no square names,
 * only what FEN itself records plus an incrementally maintained Zobrist hash
 * for repetition detection.
 *
 * board[0][0] is a8, matching board.c and FEN's reading order.
 */
typedef struct {
  Piece_t board[8][8];
  Color side_to_move;
  unsigned castling_rights; /* bitmask of CASTLE_* */
  int ep_i, ep_j;           /* en passant target square, or -1,-1 if none */
  int halfmove_clock;       /* since the last capture or pawn move */
  int fullmove_number;
  uint64_t hash; /* Zobrist key of this position, see zobrist.h */
} Position;

/* A move, carrying enough displaced state that applying it is exactly
 * reversible: unmake() restores castling rights, the en passant square, and
 * the halfmove clock, not just piece placement. */
enum {
  MOVE_NONE = 0,
  MOVE_CASTLE_KINGSIDE = 1 << 0,
  MOVE_CASTLE_QUEENSIDE = 1 << 1,
  MOVE_EN_PASSANT = 1 << 2,
  MOVE_DOUBLE_PUSH = 1 << 3
};

typedef struct {
  int from_i, from_j;
  int to_i, to_j;
  Piece_type_t moved;
  Piece_type_t captured;  /* FREE if the move is not a capture */
  Piece_type_t promotion; /* FREE unless the move promotes a pawn */
  unsigned flags;

  /* State displaced by this move, restored by unmake(). */
  unsigned prev_castling_rights;
  int prev_ep_i, prev_ep_j;
  int prev_halfmove_clock;
} Move;

// Linked list to store the player's captures
typedef struct Captures_node_s {
  Piece_t piece;
  struct Captures_node_s *p_next;
} Captures_node_t;

// Linked list to store the moves made
typedef struct History_node_s {
  char prev_pos[3];
  char next_pos[3];
  struct History_node_s *p_next;
} History_node_t;

/* Linked list of Zobrist keys, one per position reached so far in the game
 * (including the starting position), for threefold repetition. */
typedef struct Hash_node_s {
  uint64_t hash;
  struct Hash_node_s *p_next;
} Hash_node_t;

/* Everything that makes up a game in progress.
 *
 * Always passed by address. The list heads used to travel as separate
 * arguments, by value in some places and by address in others, so a callee
 * that appended to a list updated a copy of the head that its caller never
 * saw. Owning them here means there is only ever one head to update.
 */
typedef struct {
  Position position;
  Captures_node_t *p_captures_white_head;
  Captures_node_t *p_captures_black_head;
  History_node_t *p_history_head;
  Hash_node_t *p_hash_history_head;
} GameState;

#endif /* TYPES_H */
