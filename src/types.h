#ifndef TYPES_H
#define TYPES_H

/* Shared data types.
 *
 * This is the only header any other header is allowed to include. Every module
 * header includes types.h and nothing else, which keeps the include graph a
 * star rather than a web and makes header cycles impossible.
 */

#include <wchar.h>

typedef enum { WHITE, BLACK, NONE } Color;
typedef enum { PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING, FREE } Piece_type_t;

typedef struct {
  wchar_t icon;
  Color color;
  char position[3];
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

#endif /* TYPES_H */
