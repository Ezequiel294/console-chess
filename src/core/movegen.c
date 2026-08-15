#include "core/movegen.h"

#include "core/position.h"
#include "core/zobrist.h"

static const int KNIGHT_OFFSETS[8][2] = {
    {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}};
static const int KING_OFFSETS[8][2] = {
    {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};
static const int ROOK_DIRS[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
static const int BISHOP_DIRS[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

/* --- Attack detection ------------------------------------------------------
 *
 * Radiates outward from the target square along each piece's line of attack,
 * rather than generating every move by_side has and scanning the results.
 */

int is_square_attacked(const Position *pos, int i, int j, Color by_side) {
  /* Pawns: a pawn attacks diagonally forward, so an attacker of (i, j) sits
   * one rank behind it from by_side's point of view — white pawns advance
   * toward i = 0, so a white attacker is at i + 1. */
  int pawn_i = i + (by_side == WHITE ? 1 : -1);
  for (int dj = -1; dj <= 1; dj += 2) {
    int pj = j + dj;
    if (square_on_board(pawn_i, pj)) {
      Piece_t p = pos->board[pawn_i][pj];
      if (p.type == PAWN && p.color == by_side) {
        return 1;
      }
    }
  }

  for (int k = 0; k < 8; k++) {
    int ni = i + KNIGHT_OFFSETS[k][0];
    int nj = j + KNIGHT_OFFSETS[k][1];
    if (square_on_board(ni, nj)) {
      Piece_t p = pos->board[ni][nj];
      if (p.type == KNIGHT && p.color == by_side) {
        return 1;
      }
    }
  }

  for (int k = 0; k < 8; k++) {
    int ni = i + KING_OFFSETS[k][0];
    int nj = j + KING_OFFSETS[k][1];
    if (square_on_board(ni, nj)) {
      Piece_t p = pos->board[ni][nj];
      if (p.type == KING && p.color == by_side) {
        return 1;
      }
    }
  }

  for (int d = 0; d < 4; d++) {
    int ni = i + ROOK_DIRS[d][0];
    int nj = j + ROOK_DIRS[d][1];
    while (square_on_board(ni, nj)) {
      Piece_t p = pos->board[ni][nj];
      if (p.type != FREE) {
        if (p.color == by_side && (p.type == ROOK || p.type == QUEEN)) {
          return 1;
        }
        break;
      }
      ni += ROOK_DIRS[d][0];
      nj += ROOK_DIRS[d][1];
    }
  }

  for (int d = 0; d < 4; d++) {
    int ni = i + BISHOP_DIRS[d][0];
    int nj = j + BISHOP_DIRS[d][1];
    while (square_on_board(ni, nj)) {
      Piece_t p = pos->board[ni][nj];
      if (p.type != FREE) {
        if (p.color == by_side && (p.type == BISHOP || p.type == QUEEN)) {
          return 1;
        }
        break;
      }
      ni += BISHOP_DIRS[d][0];
      nj += BISHOP_DIRS[d][1];
    }
  }

  return 0;
}

int in_check(const Position *pos, Color side) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      Piece_t p = pos->board[i][j];
      if (p.type == KING && p.color == side) {
        Color enemy = (side == WHITE) ? BLACK : WHITE;
        return is_square_attacked(pos, i, j, enemy);
      }
    }
  }
  return 0; /* no king on the board: unreachable in a legal position */
}

/* --- Applying and reversing a move ----------------------------------------
 */

static void set_castling_rights(Position *pos, unsigned new_rights) {
  unsigned changed = pos->castling_rights ^ new_rights;
  if (changed & CASTLE_WK) pos->hash ^= zobrist_castle_key(CASTLE_WK);
  if (changed & CASTLE_WQ) pos->hash ^= zobrist_castle_key(CASTLE_WQ);
  if (changed & CASTLE_BK) pos->hash ^= zobrist_castle_key(CASTLE_BK);
  if (changed & CASTLE_BQ) pos->hash ^= zobrist_castle_key(CASTLE_BQ);
  pos->castling_rights = new_rights;
}

/* Castling rights lost because a king or rook moved away from, or a rook was
 * captured on, one of the four home squares — shared by make() (checking both
 * ends of the move) and here so the rule is written once. */
static unsigned revoke_rights_touching(unsigned rights, int i, int j) {
  if (i == 7 && j == 4) rights &= ~(CASTLE_WK | CASTLE_WQ);
  if (i == 0 && j == 4) rights &= ~(CASTLE_BK | CASTLE_BQ);
  if (i == 7 && j == 0) rights &= ~CASTLE_WQ;
  if (i == 7 && j == 7) rights &= ~CASTLE_WK;
  if (i == 0 && j == 0) rights &= ~CASTLE_BQ;
  if (i == 0 && j == 7) rights &= ~CASTLE_BK;
  return rights;
}

void make(Position *pos, Move move) {
  Color mover = pos->side_to_move;
  Piece_t moving = pos->board[move.from_i][move.from_j];

  pos->halfmove_clock =
      (move.captured != FREE || move.moved == PAWN) ? 0 : pos->halfmove_clock + 1;

  if (move.flags & MOVE_EN_PASSANT) {
    int cap_i = move.from_i;
    int cap_j = move.to_j;
    Color captured_color = (mover == WHITE) ? BLACK : WHITE;
    pos->hash ^= zobrist_piece_key(captured_color, PAWN, cap_i, cap_j);
    pos->board[cap_i][cap_j] = (Piece_t){.color = NONE, .type = FREE};
  } else if (move.captured != FREE) {
    Color captured_color = pos->board[move.to_i][move.to_j].color;
    pos->hash ^= zobrist_piece_key(captured_color, move.captured, move.to_i, move.to_j);
  }

  pos->hash ^= zobrist_piece_key(moving.color, moving.type, move.from_i, move.from_j);
  pos->board[move.from_i][move.from_j] = (Piece_t){.color = NONE, .type = FREE};

  Piece_t placed = moving;
  if (move.promotion != FREE) {
    placed.type = move.promotion;
  }
  pos->board[move.to_i][move.to_j] = placed;
  pos->hash ^= zobrist_piece_key(placed.color, placed.type, move.to_i, move.to_j);

  if (move.flags & (MOVE_CASTLE_KINGSIDE | MOVE_CASTLE_QUEENSIDE)) {
    int rank = move.from_i;
    int rook_from_j = (move.flags & MOVE_CASTLE_KINGSIDE) ? 7 : 0;
    int rook_to_j = (move.flags & MOVE_CASTLE_KINGSIDE) ? 5 : 3;
    Piece_t rook = pos->board[rank][rook_from_j];
    pos->hash ^= zobrist_piece_key(rook.color, rook.type, rank, rook_from_j);
    pos->board[rank][rook_from_j] = (Piece_t){.color = NONE, .type = FREE};
    pos->board[rank][rook_to_j] = rook;
    pos->hash ^= zobrist_piece_key(rook.color, rook.type, rank, rook_to_j);
  }

  unsigned new_rights = pos->castling_rights;
  new_rights = revoke_rights_touching(new_rights, move.from_i, move.from_j);
  new_rights = revoke_rights_touching(new_rights, move.to_i, move.to_j);
  set_castling_rights(pos, new_rights);

  if (pos->ep_j >= 0) {
    pos->hash ^= zobrist_ep_file_key(pos->ep_j);
  }
  if (move.flags & MOVE_DOUBLE_PUSH) {
    pos->ep_i = (move.from_i + move.to_i) / 2;
    pos->ep_j = move.from_j;
    pos->hash ^= zobrist_ep_file_key(pos->ep_j);
  } else {
    pos->ep_i = -1;
    pos->ep_j = -1;
  }

  if (mover == BLACK) {
    pos->fullmove_number++;
  }

  pos->hash ^= zobrist_side_key();
  pos->side_to_move = (mover == WHITE) ? BLACK : WHITE;
}

void unmake(Position *pos, Move move) {
  pos->hash ^= zobrist_side_key();
  pos->side_to_move = (pos->side_to_move == WHITE) ? BLACK : WHITE;
  Color mover = pos->side_to_move;

  if (mover == BLACK) {
    pos->fullmove_number--;
  }

  if (pos->ep_j >= 0) {
    pos->hash ^= zobrist_ep_file_key(pos->ep_j);
  }
  if (move.prev_ep_j >= 0) {
    pos->hash ^= zobrist_ep_file_key(move.prev_ep_j);
  }
  pos->ep_i = move.prev_ep_i;
  pos->ep_j = move.prev_ep_j;

  set_castling_rights(pos, move.prev_castling_rights);
  pos->halfmove_clock = move.prev_halfmove_clock;

  if (move.flags & (MOVE_CASTLE_KINGSIDE | MOVE_CASTLE_QUEENSIDE)) {
    int rank = move.from_i;
    int rook_from_j = (move.flags & MOVE_CASTLE_KINGSIDE) ? 7 : 0;
    int rook_to_j = (move.flags & MOVE_CASTLE_KINGSIDE) ? 5 : 3;
    Piece_t rook = pos->board[rank][rook_to_j];
    pos->hash ^= zobrist_piece_key(rook.color, rook.type, rank, rook_to_j);
    pos->board[rank][rook_to_j] = (Piece_t){.color = NONE, .type = FREE};
    pos->board[rank][rook_from_j] = rook;
    pos->hash ^= zobrist_piece_key(rook.color, rook.type, rank, rook_from_j);
  }

  Piece_t placed = pos->board[move.to_i][move.to_j];
  pos->hash ^= zobrist_piece_key(placed.color, placed.type, move.to_i, move.to_j);
  pos->board[move.to_i][move.to_j] = (Piece_t){.color = NONE, .type = FREE};

  Piece_t original = (Piece_t){.color = mover, .type = move.moved};
  pos->board[move.from_i][move.from_j] = original;
  pos->hash ^= zobrist_piece_key(original.color, original.type, move.from_i, move.from_j);

  if (move.flags & MOVE_EN_PASSANT) {
    int cap_i = move.from_i;
    int cap_j = move.to_j;
    Color captured_color = (mover == WHITE) ? BLACK : WHITE;
    pos->board[cap_i][cap_j] = (Piece_t){.color = captured_color, .type = PAWN};
    pos->hash ^= zobrist_piece_key(captured_color, PAWN, cap_i, cap_j);
  } else if (move.captured != FREE) {
    Color captured_color = (mover == WHITE) ? BLACK : WHITE;
    pos->board[move.to_i][move.to_j] = (Piece_t){.color = captured_color, .type = move.captured};
    pos->hash ^= zobrist_piece_key(captured_color, move.captured, move.to_i, move.to_j);
  }
}

/* --- Pseudo-legal generation ------------------------------------------------
 *
 * Piece geometry only: destinations a piece could reach ignoring whether the
 * move leaves its own king attacked. generate_legal_moves() below is what
 * filters that out.
 */

static void add_move(MoveList *out, const Position *pos, int fi, int fj, int ti, int tj,
                      Piece_type_t moved, Piece_type_t captured, Piece_type_t promotion,
                      unsigned flags) {
  if (out->count >= MAX_MOVES) {
    return;
  }
  Move *m = &out->moves[out->count++];
  m->from_i = fi;
  m->from_j = fj;
  m->to_i = ti;
  m->to_j = tj;
  m->moved = moved;
  m->captured = captured;
  m->promotion = promotion;
  m->flags = flags;
  m->prev_castling_rights = pos->castling_rights;
  m->prev_ep_i = pos->ep_i;
  m->prev_ep_j = pos->ep_j;
  m->prev_halfmove_clock = pos->halfmove_clock;
}

static void gen_slider(const Position *pos, MoveList *out, int i, int j,
                        const int dirs[][2], int ndirs) {
  Color us = pos->board[i][j].color;
  Piece_type_t type = pos->board[i][j].type;
  for (int d = 0; d < ndirs; d++) {
    int ni = i + dirs[d][0];
    int nj = j + dirs[d][1];
    while (square_on_board(ni, nj)) {
      Piece_t target = pos->board[ni][nj];
      if (target.type == FREE) {
        add_move(out, pos, i, j, ni, nj, type, FREE, FREE, MOVE_NONE);
      } else {
        if (target.color != us) {
          add_move(out, pos, i, j, ni, nj, type, target.type, FREE, MOVE_NONE);
        }
        break;
      }
      ni += dirs[d][0];
      nj += dirs[d][1];
    }
  }
}

static void gen_offsets(const Position *pos, MoveList *out, int i, int j,
                         const int offs[][2], int noffs) {
  Color us = pos->board[i][j].color;
  Piece_type_t type = pos->board[i][j].type;
  for (int k = 0; k < noffs; k++) {
    int ni = i + offs[k][0];
    int nj = j + offs[k][1];
    if (!square_on_board(ni, nj)) {
      continue;
    }
    Piece_t target = pos->board[ni][nj];
    if (target.type == FREE) {
      add_move(out, pos, i, j, ni, nj, type, FREE, FREE, MOVE_NONE);
    } else if (target.color != us) {
      add_move(out, pos, i, j, ni, nj, type, target.type, FREE, MOVE_NONE);
    }
  }
}

static void gen_pawn(const Position *pos, MoveList *out, int i, int j) {
  Color us = pos->board[i][j].color;
  int dir = (us == WHITE) ? -1 : 1;
  int start_rank = (us == WHITE) ? 6 : 1;
  int promo_rank = (us == WHITE) ? 0 : 7;

  int ni = i + dir;
  if (square_on_board(ni, j) && pos->board[ni][j].type == FREE) {
    if (ni == promo_rank) {
      add_move(out, pos, i, j, ni, j, PAWN, FREE, QUEEN, MOVE_NONE);
      add_move(out, pos, i, j, ni, j, PAWN, FREE, ROOK, MOVE_NONE);
      add_move(out, pos, i, j, ni, j, PAWN, FREE, BISHOP, MOVE_NONE);
      add_move(out, pos, i, j, ni, j, PAWN, FREE, KNIGHT, MOVE_NONE);
    } else {
      add_move(out, pos, i, j, ni, j, PAWN, FREE, FREE, MOVE_NONE);
    }
    if (i == start_rank) {
      int ni2 = i + 2 * dir;
      if (pos->board[ni2][j].type == FREE) {
        add_move(out, pos, i, j, ni2, j, PAWN, FREE, FREE, MOVE_DOUBLE_PUSH);
      }
    }
  }

  for (int dj = -1; dj <= 1; dj += 2) {
    int nj = j + dj;
    if (!square_on_board(ni, nj)) {
      continue;
    }
    Piece_t target = pos->board[ni][nj];
    if (target.type != FREE && target.color != us) {
      if (ni == promo_rank) {
        add_move(out, pos, i, j, ni, nj, PAWN, target.type, QUEEN, MOVE_NONE);
        add_move(out, pos, i, j, ni, nj, PAWN, target.type, ROOK, MOVE_NONE);
        add_move(out, pos, i, j, ni, nj, PAWN, target.type, BISHOP, MOVE_NONE);
        add_move(out, pos, i, j, ni, nj, PAWN, target.type, KNIGHT, MOVE_NONE);
      } else {
        add_move(out, pos, i, j, ni, nj, PAWN, target.type, FREE, MOVE_NONE);
      }
    } else if (target.type == FREE && pos->ep_i == ni && pos->ep_j == nj) {
      add_move(out, pos, i, j, ni, nj, PAWN, PAWN, FREE, MOVE_EN_PASSANT);
    }
  }
}

/* Castling: a king move of two squares toward a rook, generated only when all
 * five conditions hold. The king does not pass through b1/b8 on the queenside,
 * so only c and d (the landing square and the square crossed) are checked for
 * attack there — g and f, the landing square and the square crossed, on the
 * kingside. */
static void gen_castling(const Position *pos, MoveList *out, int i, int j) {
  Color us = pos->board[i][j].color;
  int home_rank = (us == WHITE) ? 7 : 0;
  if (i != home_rank || j != 4) {
    return;
  }
  Color enemy = (us == WHITE) ? BLACK : WHITE;
  if (is_square_attacked(pos, i, j, enemy)) {
    return; /* king is in check: no castling on either wing */
  }

  unsigned kflag = (us == WHITE) ? CASTLE_WK : CASTLE_BK;
  unsigned qflag = (us == WHITE) ? CASTLE_WQ : CASTLE_BQ;

  if ((pos->castling_rights & kflag) && pos->board[i][5].type == FREE &&
      pos->board[i][6].type == FREE && !is_square_attacked(pos, i, 5, enemy) &&
      !is_square_attacked(pos, i, 6, enemy)) {
    add_move(out, pos, i, j, i, 6, KING, FREE, FREE, MOVE_CASTLE_KINGSIDE);
  }

  if ((pos->castling_rights & qflag) && pos->board[i][1].type == FREE &&
      pos->board[i][2].type == FREE && pos->board[i][3].type == FREE &&
      !is_square_attacked(pos, i, 3, enemy) && !is_square_attacked(pos, i, 2, enemy)) {
    add_move(out, pos, i, j, i, 2, KING, FREE, FREE, MOVE_CASTLE_QUEENSIDE);
  }
}

static void generate_pseudo_legal_moves(const Position *pos, MoveList *out) {
  out->count = 0;
  Color us = pos->side_to_move;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      Piece_t p = pos->board[i][j];
      if (p.color != us) {
        continue;
      }
      switch (p.type) {
      case PAWN:
        gen_pawn(pos, out, i, j);
        break;
      case KNIGHT:
        gen_offsets(pos, out, i, j, KNIGHT_OFFSETS, 8);
        break;
      case BISHOP:
        gen_slider(pos, out, i, j, BISHOP_DIRS, 4);
        break;
      case ROOK:
        gen_slider(pos, out, i, j, ROOK_DIRS, 4);
        break;
      case QUEEN:
        gen_slider(pos, out, i, j, ROOK_DIRS, 4);
        gen_slider(pos, out, i, j, BISHOP_DIRS, 4);
        break;
      case KING:
        gen_offsets(pos, out, i, j, KING_OFFSETS, 8);
        gen_castling(pos, out, i, j);
        break;
      case FREE:
        break;
      }
    }
  }
}

/* --- Legality filter --------------------------------------------------------
 *
 * Applies each pseudo-legal candidate to a scratch copy and rejects it if the
 * mover's king ends up attacked. This one step delivers pins, check evasion,
 * and "a king may not move into check" for every piece — including the
 * en-passant-exposes-the-king case, since by the time in_check() runs, both
 * pawns are already off the rank on the scratch board.
 */

void generate_legal_moves(const Position *pos, MoveList *out) {
  MoveList pseudo;
  generate_pseudo_legal_moves(pos, &pseudo);

  out->count = 0;
  Color us = pos->side_to_move;
  for (int k = 0; k < pseudo.count; k++) {
    Position scratch = *pos;
    make(&scratch, pseudo.moves[k]);
    if (!in_check(&scratch, us) && out->count < MAX_MOVES) {
      out->moves[out->count++] = pseudo.moves[k];
    }
  }
}

void generate_legal_moves_from(const Position *pos, int i, int j, MoveList *out) {
  MoveList all;
  generate_legal_moves(pos, &all);

  out->count = 0;
  for (int k = 0; k < all.count; k++) {
    if (all.moves[k].from_i == i && all.moves[k].from_j == j && out->count < MAX_MOVES) {
      out->moves[out->count++] = all.moves[k];
    }
  }
}
