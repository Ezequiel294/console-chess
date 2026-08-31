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

/* A chess clock: the time control the game is played under, what each side
 * has left, and which side is currently running.
 *
 * Here rather than in core/chessclock.h because GameState carries one and
 * types.h is the only header another header may include — the same reason
 * Position and Move live here. Every operation on it is in
 * core/chessclock.h, which is where the reasoning about it belongs; nothing
 * outside that file writes these fields.
 *
 * initial_ms == 0 is an untimed game, and is the only representation of one.
 */
typedef struct {
  int32_t initial_ms;   /* what each side starts with; 0 means untimed */
  int32_t increment_ms; /* added to a side's clock when it completes a turn */
  /* What each side has left as of started_at_ms, indexed by Color. The
   * running side's live reading is this less the interval since; see
   * clock_remaining, which computes rather than accumulates. */
  int32_t remaining_ms[2];
  Color running;          /* the side being charged, or NONE */
  Color held;             /* the side clock_hold stopped, or NONE */
  uint64_t started_at_ms; /* when the running side's interval began */
} Chess_clock_t;

// Linked list to store the player's captures
typedef struct Captures_node_s {
  Piece_t piece;
  struct Captures_node_s *p_next;
} Captures_node_t;

// Linked list to store the moves made. Carries the full Move, not just the
// two squares, so that undo can call unmake() with exactly what make() was
// given — restoring castling rights, the en passant square, and the
// halfmove clock along with piece placement.
typedef struct History_node_s {
  char prev_pos[3];
  char next_pos[3];
  Move move;
  /* What each side had left after this move, indexed by Color — the mover's
   * own increment included, since the reading is taken at the clock press,
   * after the increment is applied. That is what a real clock shows, and it
   * is what a replay displays for the position this move reached: the
   * reading travels on the node the replay is already moving between the
   * history and redo lists, so it follows the step by construction rather
   * than through a parallel array to keep in sync. Both zero in an untimed
   * game, where nothing reads them. */
  int32_t remaining_ms[2];
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
  /* The position a save file's FEN line records: the game's opening
   * position, or the position a loaded save/slot started from. p_history_head
   * holds every move played since, so start_position plus that list is
   * exactly what save_write() writes and save_read() replays. */
  Position start_position;
  Position position;
  Captures_node_t *p_captures_white_head;
  Captures_node_t *p_captures_black_head;
  History_node_t *p_history_head;
  Hash_node_t *p_hash_history_head;
  /* Moves undone but not yet redone, most-recently-undone first. A move
   * played while this is non-empty discards it. */
  History_node_t *p_redo_head;
  /* Where this game was last saved to, or loaded from — empty until the
   * first save. Saving again reuses this path, so repeated saves of the same
   * game update one file instead of piling up a new one each time. */
  char save_path[300];

  /* The game's identity, independent of its file's name — empty until the
   * first save, assigned then and kept for the game's whole life. Six hex
   * digits plus a NUL; see app/save.h. */
  char id[7];
  /* What the game is called, and whether the player typed that name or it
   * stands in for a date the game is willing to move forward on its own.
   * Up to 32 codepoints of UTF-8; see app/save.h's SAVE_NAME_MAX_CODEPOINTS.
   * Populated from the filename on load, from the prompt on a first save or
   * rename. */
  char name[129];
  int name_given;
  /* How the game ended, or OUTCOME_IN_PROGRESS while it is still being
   * played — kept as raw fields rather than an Outcome_t, since types.h
   * cannot include core/outcome.h without a cycle (outcome.h includes
   * types.h for Color and Position). See core/outcome.h for what
   * result_reason's values mean. */
  int result_reason;
  Color result_winner;

  /* The game's clock: its time control, both remaining times, and which side
   * is running. An untimed game leaves this zeroed, which is exactly what
   * clock_init(&clock, 0, 0) produces — see core/chessclock.h. */
  Chess_clock_t clock;
} GameState;

#endif /* TYPES_H */
