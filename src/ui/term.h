#ifndef TERM_H
#define TERM_H

#include "types.h"

/* The terminal as a resource.
 *
 * Owns every mode change made to the user's terminal and, more importantly,
 * owns undoing them. Restoration is registered before the first mode change is
 * made, so there is no window in which the program can die having changed the
 * terminal but not yet arranged to change it back.
 *
 * Nothing here formats output. term.c writes fixed escape sequences with
 * write(2); everything the game draws goes through render.c.
 */

typedef struct {
  int cols;
  int rows;
} Term_size_t;

/* Both standard input and standard output are interactive terminals.
 * Checked before any mode change: a redirected run must leave no escape
 * sequences in the redirected stream. */
int term_is_interactive(void);

/* Saves the current terminal attributes and arms restoration: atexit plus
 * handlers for SIGINT, SIGTERM and SIGSEGV. Call once, before term_enter or
 * term_probe_glyph_width. Returns 1 on success, 0 if the attributes could not
 * be read. */
int term_init(void);

/* Raw mode plus the full-screen entry sequence: alternate screen, hidden
 * cursor, alternate scroll off, bracketed paste on. Returns 1 on success. */
int term_enter(void);

/* Undoes everything term_enter did and restores the saved attributes.
 * Idempotent, safe from a signal handler, and safe to call having never
 * entered. */
void term_restore(void);

/* Current size in character cells. Falls back to 80x24 if the query fails. */
Term_size_t term_size(void);

/* Non-zero once per size change. Reading it clears it. */
int term_take_resize(void);

/* How many cells this terminal uses to draw sample, a UTF-8 encoded piece
 * glyph. Must be called after term_init and before term_enter, since it draws
 * on the primary screen. Returns 1 or 2; returns TERM_GLYPH_WIDTH_DEFAULT if
 * the terminal does not answer within the bounded wait. */
int term_probe_glyph_width(const char *sample);

/* Whether the terminal is worth spending colour on. There is no reliable
 * query for this — unlike glyph width, nothing answers back — so it is a
 * heuristic read from the environment: honour NO_COLOR if the user set it,
 * and treat TERM being unset or "dumb" as no colour. Everything else is
 * assumed to support it, since the xterm-256 palette this program uses is
 * decades old and near-universal among terminals that answer to anything
 * else. */
int term_supports_color(void);

/* The width assumed when the terminal will not say. Two, because the terminals
 * that answer overwhelmingly answer two for these codepoints, and because a
 * board drawn two-wide on a one-wide terminal has gaps while the reverse
 * overlaps. */
#define TERM_GLYPH_WIDTH_DEFAULT 2

/* Writes n bytes to the terminal, retrying short writes and EINTR. The only
 * output path in the program. */
void term_write(const char *buf, unsigned long n);

/* Reads up to n bytes from the terminal, waiting at most timeout_ms
 * (negative to wait indefinitely). Returns the byte count, 0 on timeout,
 * TERM_READ_EOF at end of input, or TERM_READ_INTR if a signal arrived.
 *
 * input.c is the only caller once the game is running; see the note on the
 * single-reader rule in input.c. */
long term_read(char *buf, unsigned long n, int timeout_ms);

#define TERM_READ_EOF (-1L)
#define TERM_READ_INTR (-2L)

#endif /* TERM_H */
