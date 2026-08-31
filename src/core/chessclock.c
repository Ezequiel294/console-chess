#include "core/chessclock.h"

#include <stdio.h>

/* The interval the running side has been running for, in milliseconds.
 *
 * now_ms before started_at_ms charges nothing rather than wrapping: the
 * timestamps are unsigned, and a caller handing back a reading from before
 * the interval began — a repeated now, a tick computed a moment earlier —
 * must not produce an enormous elapsed time. */
static int32_t elapsed_ms(uint64_t started_at_ms, uint64_t now_ms) {
  if (now_ms <= started_at_ms) {
    return 0;
  }
  uint64_t delta = now_ms - started_at_ms;
  /* Clamped: anything this large has already taken every clock to zero, and
   * the caller only ever compares the result against a remaining time. */
  if (delta > (uint64_t)INT32_MAX) {
    return INT32_MAX;
  }
  return (int32_t)delta;
}

void clock_init(Chess_clock_t *c, int32_t initial_ms, int32_t increment_ms) {
  if (initial_ms < 0) {
    initial_ms = 0;
  }
  if (increment_ms < 0) {
    increment_ms = 0;
  }
  c->initial_ms = initial_ms;
  c->increment_ms = increment_ms;
  c->remaining_ms[WHITE] = initial_ms;
  c->remaining_ms[BLACK] = initial_ms;
  c->running = NONE;
  c->held = NONE;
  c->started_at_ms = 0;
}

int clock_is_timed(const Chess_clock_t *c) { return c->initial_ms > 0; }

void clock_start(Chess_clock_t *c, Color side, uint64_t now_ms) {
  if (!clock_is_timed(c) || side == NONE) {
    return;
  }
  c->running = side;
  c->held = NONE;
  c->started_at_ms = now_ms;
}

/* Charges the running side up to now_ms and stores the result, leaving
 * nothing running. The one place remaining_ms is ever written from an
 * elapsed interval, so "computed, never accumulated" holds everywhere else. */
static Color settle(Chess_clock_t *c, uint64_t now_ms) {
  Color side = c->running;
  if (side == NONE) {
    return NONE;
  }
  c->remaining_ms[side] = clock_remaining(c, side, now_ms);
  c->running = NONE;
  return side;
}

void clock_press(Chess_clock_t *c, uint64_t now_ms) {
  if (!clock_is_timed(c)) {
    return;
  }
  Color mover = settle(c, now_ms);
  if (mover == NONE) {
    return;
  }
  /* The increment lands on the mover, after they have been charged for the
   * turn — which is what a real clock does, and what makes the reading
   * stored for this move the one the players saw. */
  c->remaining_ms[mover] += c->increment_ms;

  Color other = (mover == WHITE) ? BLACK : WHITE;
  clock_start(c, other, now_ms);
}

void clock_sync(Chess_clock_t *c, uint64_t now_ms) {
  Color side = settle(c, now_ms);
  if (side != NONE) {
    clock_start(c, side, now_ms);
  }
}

void clock_stop(Chess_clock_t *c, uint64_t now_ms) {
  settle(c, now_ms);
  c->held = NONE;
}

void clock_hold(Chess_clock_t *c, uint64_t now_ms) {
  if (!clock_is_timed(c) || c->running == NONE) {
    return;
  }
  c->held = settle(c, now_ms);
}

void clock_resume(Chess_clock_t *c, uint64_t now_ms) {
  if (c->held == NONE) {
    return;
  }
  Color side = c->held;
  c->held = NONE;
  clock_start(c, side, now_ms);
}

int clock_is_held(const Chess_clock_t *c) { return c->held != NONE; }

Color clock_running_side(const Chess_clock_t *c) { return c->running; }

int32_t clock_remaining(const Chess_clock_t *c, Color side, uint64_t now_ms) {
  if (side == NONE) {
    return 0;
  }
  int32_t base = c->remaining_ms[side];
  if (side == c->running) {
    int32_t spent = elapsed_ms(c->started_at_ms, now_ms);
    base = (spent >= base) ? 0 : base - spent;
  }
  return base < 0 ? 0 : base;
}

Color clock_expired_side(const Chess_clock_t *c, uint64_t now_ms) {
  if (!clock_is_timed(c) || c->running == NONE) {
    return NONE;
  }
  return (clock_remaining(c, c->running, now_ms) == 0) ? c->running : NONE;
}

void clock_format_control(const Chess_clock_t *c, char *buf, size_t len) {
  if (len == 0) {
    return;
  }
  if (!clock_is_timed(c)) {
    buf[0] = '\0';
    return;
  }
  int32_t seconds = c->initial_ms / 1000;
  int32_t increment = c->increment_ms / 1000;
  if (seconds % 60 == 0) {
    snprintf(buf, len, "%d+%d", seconds / 60, increment);
  } else {
    snprintf(buf, len, "%d:%02d+%d", seconds / 60, seconds % 60, increment);
  }
}

void clock_format(int32_t ms, char *buf, size_t len) {
  if (len == 0) {
    return;
  }
  if (ms < 0) {
    ms = 0;
  }

  /* Every division truncates, so a reading never claims time a side does not
   * have: "0:01" means at least one whole second is left. */
  if (ms < 10000) {
    snprintf(buf, len, "%d.%d", ms / 1000, (ms % 1000) / 100);
    return;
  }

  int32_t total_seconds = ms / 1000;
  int32_t hours = total_seconds / 3600;
  int32_t minutes = (total_seconds / 60) % 60;
  int32_t seconds = total_seconds % 60;

  if (hours > 0) {
    snprintf(buf, len, "%d:%02d:%02d", hours, minutes, seconds);
  } else {
    snprintf(buf, len, "%d:%02d", minutes, seconds);
  }
}
