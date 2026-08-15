#ifndef NOTATION_H
#define NOTATION_H

#include "types.h"

/* Reads and writes positions and moves in the standard chess text formats:
 * FEN for a complete position, coordinate text for a single move, and SAN for
 * a move in the context of the position it was played from.
 *
 * Positions written here are also what the save format uses and what test
 * fixtures are expressed as, so a round trip through fen_parse/fen_write must
 * be exact.
 */

/* Long enough for any legal FEN: 8 ranks of up to 8 characters each plus 7
 * separators for placement, then the five remaining fields and their spaces,
 * with generous room for the numeric fields. */
#define FEN_MAX_LEN 96

/* Parses a complete position from a single line of text: piece placement,
 * side to move, castling rights, en passant target, halfmove clock, fullmove
 * number, in that order and separated by single spaces.
 *
 * Returns 1 and writes *out on success. Returns 0 and leaves *out untouched on
 * any malformed input: a field count other than six, a rank that does not sum
 * to exactly 8 squares, an unrecognised piece letter, or placement missing
 * either king. */
int fen_parse(const char *fen, Position *out);

/* Writes *pos into out, which must have room for at least FEN_MAX_LEN bytes
 * including the terminating NUL. */
void fen_write(const Position *pos, char out[FEN_MAX_LEN]);

/* Long enough for a square pair plus a promotion letter: "e7e8q". */
#define COORD_MAX_LEN 6

/* Writes move as coordinate text: origin square, destination square, and for
 * a promotion a trailing piece letter. A castling move is written as the
 * king's own origin and destination, since that is exactly what Move already
 * records — nothing special-cased. */
void move_to_coord(Move move, char out[COORD_MAX_LEN]);

/* Resolves coordinate text against pos's legal moves. Returns 1 and writes
 * *out on a match, 0 if coord is malformed or names no legal move. */
int coord_to_move(const Position *pos, const char *coord, Move *out);

/* Long enough for any SAN move: piece letter, disambiguation by file and
 * rank, capture marker, destination, promotion, and a check or checkmate
 * marker — or "O-O-O" plus a marker, the longest castling form. */
#define SAN_MAX_LEN 12

/* Writes move in standard algebraic notation, in the context of the position
 * it is played from: the piece letter, capture marker, destination,
 * promotion, castling, and a check or checkmate marker, disambiguated by file
 * or by rank when more than one like piece could reach the same square. */
void move_to_san(const Position *pos, Move move, char out[SAN_MAX_LEN]);

#endif /* NOTATION_H */
