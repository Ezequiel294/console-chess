#ifndef CHESSCLOCK_H
#define CHESSCLOCK_H

#include "types.h"

#include <stddef.h>

/* A chess clock: a time control, both sides' remaining time, and which side
 * is currently running.
 *
 * Every function here takes now_ms — a monotonic millisecond reading — as a
 * parameter and reads no clock of its own, which is what keeps this in
 * core/ alongside the rest of the I/O-free rules layer and what makes the
 * whole of a clock's behaviour (increment, flag fall, a mid-turn save and
 * resume) testable by passing fabricated timestamps rather than by sleeping.
 * The one thing that genuinely needs the operating system, "what time is
 * it", is term_now_ms() in ui/term.c and is the program's only caller of it.
 *
 * Chess_clock_t itself lives in types.h, for the same reason every other
 * shared struct does: GameState carries one, and types.h is the only header
 * another header may include (see types.h).
 *
 * An untimed game is initial_ms == 0 and nothing else — there is no second
 * flag that could disagree with it. clock_is_timed() is the only test for
 * it, and every operation below is a no-op on an untimed clock, so an
 * untimed clock never runs, never expires, and never has anything added to
 * it.
 */

/* Sets the time control and puts both sides on initial_ms with neither
 * running. initial_ms 0 is an untimed game; a negative initial_ms or
 * increment_ms is clamped to 0. */
void clock_init(Chess_clock_t *c, int32_t initial_ms, int32_t increment_ms);

/* Whether this game has a clock at all. */
int clock_is_timed(const Chess_clock_t *c);

/* Starts side's clock at now_ms, from whatever it has left. The other side
 * is left exactly as it is. Used at the start of a turn — the first move of
 * a game, or the side to move in a game just loaded — where there is no
 * press to make. */
void clock_start(Chess_clock_t *c, Color side, uint64_t now_ms);

/* The clock press: charges the running side up to now_ms, adds its
 * increment, and starts the other side at now_ms — one moment, in that
 * order. Does nothing if no side is running. */
void clock_press(Chess_clock_t *c, uint64_t now_ms);

/* Stops the clock for good: the running side is charged up to now_ms and its
 * remaining time stored, and neither side runs afterwards. For a game that
 * has ended — a game-ending move, a resignation, a flag fall — where both
 * clocks stop at that moment and nothing is ever going to start them again. */
void clock_stop(Chess_clock_t *c, uint64_t now_ms);

/* Brings the stored remaining times up to now_ms without interrupting play:
 * the running side is charged for the interval so far and its reading
 * stored, and its interval restarts from now_ms. Nothing is gained and
 * nothing is lost.
 *
 * For writing a save file, which records the reading at the moment of
 * writing: a game saved mid-turn must resume with the mover's time already
 * partly spent rather than returned to them. */
void clock_sync(Chess_clock_t *c, uint64_t now_ms);

/* Stops charging without starting anybody: the running side is charged up to
 * now_ms and its remaining time stored, and the clock remembers which side
 * it was so clock_resume() can start it again. For the one case where the
 * game is not playable at all — the terminal too small to draw it — where
 * neither side may be charged for the gap. */
void clock_hold(Chess_clock_t *c, uint64_t now_ms);

/* Restarts the side clock_hold() stopped, its interval beginning at now_ms
 * so nothing is charged for the time in between. Does nothing if the clock
 * was not held. */
void clock_resume(Chess_clock_t *c, uint64_t now_ms);

/* Whether clock_hold() has stopped this clock and clock_resume() has not yet
 * restarted it. */
int clock_is_held(const Chess_clock_t *c);

/* The side currently being charged, or NONE. */
Color clock_running_side(const Chess_clock_t *c);

/* What side has left at now_ms: its stored remaining time, less the interval
 * it has been running for if it is the side running. Computed from two
 * timestamps rather than accumulated per tick, so a tick that is late,
 * early, missed, or delivered twice cannot corrupt the reading. Never
 * negative, and never depends on now_ms moving forward: a now_ms before the
 * running side's interval began charges nothing. */
int32_t clock_remaining(const Chess_clock_t *c, Color side, uint64_t now_ms);

/* The side whose clock has reached zero at now_ms, or NONE. Only ever the
 * running side: a side that is not running cannot run out of time, and an
 * untimed clock never expires. */
Color clock_expired_side(const Chess_clock_t *c, uint64_t now_ms);

/* Room for the longest reading clock_format can produce. */
#define CLOCK_FORMAT_MAX 16

/* Writes ms as a time: tenths of a second below ten seconds ("9.4"),
 * minutes and seconds below an hour ("9:47"), hours above ("1:05:00").
 * Always truncated, never rounded up, so a reading of "0:01" really does
 * have at least a second behind it. A negative ms formats as zero. */
void clock_format(int32_t ms, char *buf, size_t len);

/* Room for the longest control clock_format_control can produce. */
#define CLOCK_CONTROL_MAX 24

/* The time control as players name it: "10+0", "3+2", "0:30+1". An untimed
 * clock produces an empty string, so a caller can print it unconditionally
 * and have an untimed game show nothing rather than a placeholder. */
void clock_format_control(const Chess_clock_t *c, char *buf, size_t len);

#endif /* CHESSCLOCK_H */
