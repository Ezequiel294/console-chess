#include "core/notation.h"

#include "core/board.h"
#include "core/movegen.h"
#include "core/position.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static char piece_letter(Piece_type_t type, Color color) {
  char letter;
  switch (type) {
  case PAWN: letter = 'p'; break;
  case ROOK: letter = 'r'; break;
  case KNIGHT: letter = 'n'; break;
  case BISHOP: letter = 'b'; break;
  case QUEEN: letter = 'q'; break;
  case KING: letter = 'k'; break;
  default: return '\0';
  }
  return (color == WHITE) ? (char)toupper((unsigned char)letter) : letter;
}

static int letter_to_piece(char c, Piece_type_t *type, Color *color) {
  Color col = isupper((unsigned char)c) ? WHITE : BLACK;
  switch (tolower((unsigned char)c)) {
  case 'p': *type = PAWN; break;
  case 'r': *type = ROOK; break;
  case 'n': *type = KNIGHT; break;
  case 'b': *type = BISHOP; break;
  case 'q': *type = QUEEN; break;
  case 'k': *type = KING; break;
  default: return 0;
  }
  *color = col;
  return 1;
}

/* --- Parsing -------------------------------------------------------------- */

static int parse_placement(const char *field, Piece_t board[8][8]) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      board[i][j] = (Piece_t){.color = NONE, .type = FREE};
    }
  }

  int i = 0;
  int j = 0;
  int seen_ranks = 1;

  for (const char *p = field; *p != '\0'; p++) {
    if (*p == '/') {
      if (j != 8) {
        return 0; /* rank did not sum to exactly 8 squares */
      }
      i++;
      j = 0;
      seen_ranks++;
      if (i >= 8) {
        return 0; /* more than 8 ranks */
      }
      continue;
    }
    if (*p >= '1' && *p <= '8') {
      j += *p - '0';
      if (j > 8) {
        return 0;
      }
      continue;
    }
    Piece_type_t type;
    Color color;
    if (!letter_to_piece(*p, &type, &color) || j >= 8) {
      return 0; /* unknown piece letter, or a rank already full */
    }
    board[i][j] = (Piece_t){.color = color, .type = type};
    j++;
  }

  if (j != 8 || seen_ranks != 8) {
    return 0;
  }

  int white_king = 0;
  int black_king = 0;
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      if (board[r][c].type == KING) {
        if (board[r][c].color == WHITE) white_king = 1;
        if (board[r][c].color == BLACK) black_king = 1;
      }
    }
  }
  return white_king && black_king;
}

static int parse_castling(const char *field, unsigned *out) {
  unsigned rights = 0;
  if (strcmp(field, "-") == 0) {
    *out = 0;
    return 1;
  }
  if (field[0] == '\0') {
    return 0;
  }
  for (const char *p = field; *p != '\0'; p++) {
    switch (*p) {
    case 'K': rights |= CASTLE_WK; break;
    case 'Q': rights |= CASTLE_WQ; break;
    case 'k': rights |= CASTLE_BK; break;
    case 'q': rights |= CASTLE_BQ; break;
    default: return 0;
    }
  }
  *out = rights;
  return 1;
}

static int parse_uint_field(const char *field, int *out) {
  if (field[0] == '\0') {
    return 0;
  }
  int value = 0;
  for (const char *p = field; *p != '\0'; p++) {
    if (*p < '0' || *p > '9') {
      return 0;
    }
    value = value * 10 + (*p - '0');
  }
  *out = value;
  return 1;
}

int fen_parse(const char *fen, Position *out) {
  if (fen == NULL || strlen(fen) >= FEN_MAX_LEN) {
    return 0;
  }

  char buf[FEN_MAX_LEN];
  strcpy(buf, fen);

  /* Split on single spaces by hand rather than strtok_r, which is POSIX and
   * not part of the C17 standard this project builds against. */
  char *fields[6];
  int nfields = 0;
  char *p = buf;
  while (*p != '\0') {
    if (nfields == 6) {
      return 0; /* too many fields */
    }
    fields[nfields++] = p;
    while (*p != '\0' && *p != ' ') {
      p++;
    }
    if (*p == ' ') {
      *p = '\0';
      p++;
      if (*p == '\0' || *p == ' ') {
        return 0; /* trailing or doubled space */
      }
    }
  }
  if (nfields != 6) {
    return 0;
  }

  Position pos = {0};

  if (!parse_placement(fields[0], pos.board)) {
    return 0;
  }

  if (strcmp(fields[1], "w") == 0) {
    pos.side_to_move = WHITE;
  } else if (strcmp(fields[1], "b") == 0) {
    pos.side_to_move = BLACK;
  } else {
    return 0;
  }

  if (!parse_castling(fields[2], &pos.castling_rights)) {
    return 0;
  }

  if (strcmp(fields[3], "-") == 0) {
    pos.ep_i = -1;
    pos.ep_j = -1;
  } else if (square_to_index(fields[3], &pos.ep_i, &pos.ep_j)) {
    /* accepted as given */
  } else {
    return 0;
  }

  if (!parse_uint_field(fields[4], &pos.halfmove_clock)) {
    return 0;
  }
  if (!parse_uint_field(fields[5], &pos.fullmove_number) || pos.fullmove_number < 1) {
    return 0;
  }

  position_compute_hash(&pos);
  *out = pos;
  return 1;
}

/* --- Writing ---------------------------------------------------------------
 */

void fen_write(const Position *pos, char out[FEN_MAX_LEN]) {
  char *w = out;

  for (int i = 0; i < 8; i++) {
    int empty = 0;
    for (int j = 0; j < 8; j++) {
      Piece_t p = pos->board[i][j];
      if (p.type == FREE) {
        empty++;
        continue;
      }
      if (empty > 0) {
        *w++ = (char)('0' + empty);
        empty = 0;
      }
      *w++ = piece_letter(p.type, p.color);
    }
    if (empty > 0) {
      *w++ = (char)('0' + empty);
    }
    if (i < 7) {
      *w++ = '/';
    }
  }

  *w++ = ' ';
  *w++ = (pos->side_to_move == WHITE) ? 'w' : 'b';
  *w++ = ' ';

  if (pos->castling_rights == 0) {
    *w++ = '-';
  } else {
    if (pos->castling_rights & CASTLE_WK) *w++ = 'K';
    if (pos->castling_rights & CASTLE_WQ) *w++ = 'Q';
    if (pos->castling_rights & CASTLE_BK) *w++ = 'k';
    if (pos->castling_rights & CASTLE_BQ) *w++ = 'q';
  }
  *w++ = ' ';

  if (pos->ep_i < 0) {
    *w++ = '-';
  } else {
    char sq[3];
    index_to_square(pos->ep_i, pos->ep_j, sq);
    *w++ = sq[0];
    *w++ = sq[1];
  }

  *w = '\0';

  /* The clocks are appended with snprintf rather than folded into the
   * character-at-a-time loop above: they are the one part of the record whose
   * width is not fixed by chess itself. */
  char tail[32];
  snprintf(tail, sizeof(tail), " %d %d", pos->halfmove_clock, pos->fullmove_number);
  strcat(out, tail);
}

/* --- Coordinate move text ---------------------------------------------------
 */

void move_to_coord(Move move, char out[COORD_MAX_LEN]) {
  char from[3], to[3];
  index_to_square(move.from_i, move.from_j, from);
  index_to_square(move.to_i, move.to_j, to);
  out[0] = from[0];
  out[1] = from[1];
  out[2] = to[0];
  out[3] = to[1];
  int idx = 4;
  if (move.promotion != FREE) {
    out[idx++] = piece_letter(move.promotion, BLACK); /* lowercase */
  }
  out[idx] = '\0';
}

int coord_to_move(const Position *pos, const char *coord, Move *out) {
  size_t len = strlen(coord);
  if (len != 4 && len != 5) {
    return 0;
  }

  char from[3] = {coord[0], coord[1], '\0'};
  char to[3] = {coord[2], coord[3], '\0'};
  int fi, fj, ti, tj;
  if (!square_to_index(from, &fi, &fj) || !square_to_index(to, &ti, &tj)) {
    return 0;
  }

  Piece_type_t promo = FREE;
  if (len == 5) {
    Color unused_color;
    if (!letter_to_piece(coord[4], &promo, &unused_color) ||
        (promo != QUEEN && promo != ROOK && promo != BISHOP && promo != KNIGHT)) {
      return 0;
    }
  }

  MoveList moves;
  generate_legal_moves(pos, &moves);
  for (int k = 0; k < moves.count; k++) {
    Move m = moves.moves[k];
    if (m.from_i == fi && m.from_j == fj && m.to_i == ti && m.to_j == tj &&
        m.promotion == promo) {
      *out = m;
      return 1;
    }
  }
  return 0;
}

/* --- SAN --------------------------------------------------------------------
 */

/* Whether some other legal move in pos reaches the same square with the same
 * piece type, and if so, whether any of those share this move's file or
 * rank — file alone disambiguates unless another candidate shares it, and
 * likewise for rank. */
static void find_ambiguity(const Position *pos, Move move, int *ambiguous, int *same_file,
                            int *same_rank) {
  *ambiguous = 0;
  *same_file = 0;
  *same_rank = 0;

  MoveList all;
  generate_legal_moves(pos, &all);
  for (int k = 0; k < all.count; k++) {
    Move o = all.moves[k];
    if (o.to_i != move.to_i || o.to_j != move.to_j || o.moved != move.moved) {
      continue;
    }
    if (o.from_i == move.from_i && o.from_j == move.from_j) {
      continue;
    }
    *ambiguous = 1;
    if (o.from_j == move.from_j) {
      *same_file = 1;
    }
    if (o.from_i == move.from_i) {
      *same_rank = 1;
    }
  }
}

void move_to_san(const Position *pos, Move move, char out[SAN_MAX_LEN]) {
  char *w = out;

  if (move.flags & MOVE_CASTLE_KINGSIDE) {
    memcpy(w, "O-O", 3);
    w += 3;
  } else if (move.flags & MOVE_CASTLE_QUEENSIDE) {
    memcpy(w, "O-O-O", 5);
    w += 5;
  } else {
    char from[3], to[3];
    index_to_square(move.from_i, move.from_j, from);
    index_to_square(move.to_i, move.to_j, to);

    if (move.moved != PAWN) {
      *w++ = piece_letter(move.moved, WHITE); /* uppercase, colour-neutral */

      int ambiguous, same_file, same_rank;
      find_ambiguity(pos, move, &ambiguous, &same_file, &same_rank);
      if (ambiguous) {
        if (!same_file) {
          *w++ = from[0];
        } else if (!same_rank) {
          *w++ = from[1];
        } else {
          *w++ = from[0];
          *w++ = from[1];
        }
      }
    } else if (move.captured != FREE) {
      *w++ = from[0]; /* a pawn capture is prefixed by its origin file */
    }

    if (move.captured != FREE) {
      *w++ = 'x';
    }
    *w++ = to[0];
    *w++ = to[1];

    if (move.promotion != FREE) {
      *w++ = '=';
      *w++ = piece_letter(move.promotion, WHITE);
    }
  }

  Position after = *pos;
  make(&after, move);
  if (in_check(&after, after.side_to_move)) {
    MoveList replies;
    generate_legal_moves(&after, &replies);
    *w++ = (replies.count == 0) ? '#' : '+';
  }

  *w = '\0';
}
