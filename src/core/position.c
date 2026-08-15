#include "core/position.h"

#include "core/board.h"
#include "core/zobrist.h"

void position_compute_hash(Position *pos) { pos->hash = zobrist_hash(pos); }

void position_init(Position *pos) {
  init_board(pos->board);
  pos->side_to_move = WHITE;
  pos->castling_rights = CASTLE_ALL;
  pos->ep_i = -1;
  pos->ep_j = -1;
  pos->halfmove_clock = 0;
  pos->fullmove_number = 1;
  position_compute_hash(pos);
}

int square_on_board(int i, int j) { return i >= 0 && i < 8 && j >= 0 && j < 8; }

int square_is_light(int i, int j) { return (i + j) % 2 == 0; }
