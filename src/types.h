#ifndef TYPES_H
#define TYPES_H

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

/* Everything that makes up a game in progress.
 *
 * Always passed by address. The list heads used to travel as separate
 * arguments, by value in some places and by address in others, so a callee
 * that appended to a list updated a copy of the head that its caller never
 * saw. Owning them here means there is only ever one head to update.
 */
typedef struct {
  Piece_t board[8][8];
  Captures_node_t *p_captures_white_head;
  Captures_node_t *p_captures_black_head;
  History_node_t *p_history_head;
  int moves;
} GameState;

#endif /* TYPES_H */
