#include "test.h"

#include "core/chessclock.h"

#include <string.h>

/* Every timestamp here is fabricated. The clock reads no clock of its own,
 * so its whole behaviour — increment, flag fall, a held interval — is
 * exercised without sleeping and without a terminal. */

#define MIN_MS(m) ((int32_t)((m) * 60 * 1000))

static void test_init_and_is_timed(void) {
  Chess_clock_t c;

  clock_init(&c, MIN_MS(10), 2000);
  TEST_CHECK(clock_is_timed(&c));
  TEST_CHECK(c.remaining_ms[WHITE] == MIN_MS(10));
  TEST_CHECK(c.remaining_ms[BLACK] == MIN_MS(10));
  TEST_CHECK(c.running == NONE);
  TEST_CHECK(clock_remaining(&c, WHITE, 999999) == MIN_MS(10));
  TEST_CHECK(clock_remaining(&c, BLACK, 999999) == MIN_MS(10));

  /* Untimed is initial_ms == 0 and nothing else: no operation on it starts
   * anything, adds anything, or expires. */
  clock_init(&c, 0, 5000);
  TEST_CHECK(!clock_is_timed(&c));
  clock_start(&c, WHITE, 1000);
  TEST_CHECK(c.running == NONE);
  clock_press(&c, 100000);
  TEST_CHECK(c.running == NONE);
  TEST_CHECK(c.remaining_ms[WHITE] == 0 && c.remaining_ms[BLACK] == 0);
  TEST_CHECK(clock_expired_side(&c, 100000000) == NONE);

  /* A negative control is clamped rather than stored. */
  clock_init(&c, -5, -5);
  TEST_CHECK(c.initial_ms == 0 && c.increment_ms == 0);
}

/* clock_remaining is base - (now - started_at): a function of two
 * timestamps, so repeated, out-of-order and stale readings are all
 * consistent and none is ever negative. */
static void test_remaining_is_computed(void) {
  Chess_clock_t c;
  clock_init(&c, MIN_MS(1), 0);
  clock_start(&c, WHITE, 10000);

  TEST_CHECK(clock_remaining(&c, WHITE, 10000) == MIN_MS(1));
  TEST_CHECK(clock_remaining(&c, WHITE, 15000) == MIN_MS(1) - 5000);
  /* Repeated: the same answer, not a second subtraction. */
  TEST_CHECK(clock_remaining(&c, WHITE, 15000) == MIN_MS(1) - 5000);
  TEST_CHECK(clock_remaining(&c, WHITE, 15000) == MIN_MS(1) - 5000);
  /* Out of order: going back does not hand time back, and a now before the
   * interval began charges nothing rather than wrapping. */
  TEST_CHECK(clock_remaining(&c, WHITE, 12000) == MIN_MS(1) - 2000);
  TEST_CHECK(clock_remaining(&c, WHITE, 10000) == MIN_MS(1));
  TEST_CHECK(clock_remaining(&c, WHITE, 0) == MIN_MS(1));
  TEST_CHECK(clock_remaining(&c, WHITE, 15000) == MIN_MS(1) - 5000);

  /* Past the end: floored at zero, never negative, however far past. */
  TEST_CHECK(clock_remaining(&c, WHITE, 70000) == 0);
  TEST_CHECK(clock_remaining(&c, WHITE, 10000000) == 0);
  TEST_CHECK(clock_remaining(&c, WHITE, (uint64_t)1 << 40) == 0);

  /* The still side does not move at all. */
  TEST_CHECK(clock_remaining(&c, BLACK, 10000) == MIN_MS(1));
  TEST_CHECK(clock_remaining(&c, BLACK, 10000000) == MIN_MS(1));
}

static void test_press(void) {
  Chess_clock_t c;
  clock_init(&c, MIN_MS(10), 2000);
  clock_start(&c, WHITE, 1000);

  int32_t before = clock_remaining(&c, WHITE, 1000);
  clock_press(&c, 6000); /* five seconds spent */

  /* before - elapsed + increment for the mover, untouched for the other. */
  TEST_CHECK(clock_remaining(&c, WHITE, 6000) == before - 5000 + 2000);
  TEST_CHECK(clock_remaining(&c, BLACK, 6000) == MIN_MS(10));
  TEST_CHECK(c.running == BLACK);
  /* The opponent starts at the press and not before it. */
  TEST_CHECK(clock_remaining(&c, BLACK, 9000) == MIN_MS(10) - 3000);
  /* The mover is still now, so their reading does not move. */
  TEST_CHECK(clock_remaining(&c, WHITE, 9000) == before - 5000 + 2000);

  /* A press with no time elapsed loses nothing. */
  clock_init(&c, MIN_MS(5), 0);
  clock_start(&c, WHITE, 500);
  clock_press(&c, 500);
  TEST_CHECK(clock_remaining(&c, WHITE, 500) == MIN_MS(5));
  TEST_CHECK(c.running == BLACK);

  /* A zero increment adds nothing. */
  clock_init(&c, MIN_MS(5), 0);
  clock_start(&c, WHITE, 0);
  clock_press(&c, 3000);
  TEST_CHECK(clock_remaining(&c, WHITE, 3000) == MIN_MS(5) - 3000);

  /* A press with nobody running does nothing. */
  clock_init(&c, MIN_MS(5), 1000);
  clock_press(&c, 3000);
  TEST_CHECK(c.running == NONE);
  TEST_CHECK(clock_remaining(&c, WHITE, 3000) == MIN_MS(5));
  TEST_CHECK(clock_remaining(&c, BLACK, 3000) == MIN_MS(5));
}

/* The terminal-too-small case: the interval is restarted rather than the gap
 * subtracted, so neither side is charged for the time the game could not be
 * played. */
static void test_hold_and_resume(void) {
  Chess_clock_t c;
  clock_init(&c, MIN_MS(1), 0);
  clock_start(&c, WHITE, 1000);

  clock_hold(&c, 4000); /* three seconds spent before the hold */
  TEST_CHECK(clock_is_held(&c));
  TEST_CHECK(c.running == NONE);
  TEST_CHECK(clock_expired_side(&c, 10000000) == NONE);
  TEST_CHECK(clock_remaining(&c, WHITE, 4000) == MIN_MS(1) - 3000);
  TEST_CHECK(clock_remaining(&c, WHITE, 900000) == MIN_MS(1) - 3000);

  clock_resume(&c, 900000);
  TEST_CHECK(!clock_is_held(&c));
  TEST_CHECK(c.running == WHITE);
  TEST_CHECK(clock_remaining(&c, WHITE, 900000) == MIN_MS(1) - 3000);
  TEST_CHECK(clock_remaining(&c, WHITE, 902000) == MIN_MS(1) - 5000);
  TEST_CHECK(clock_remaining(&c, BLACK, 902000) == MIN_MS(1));

  /* A resume with nothing held does nothing. */
  clock_resume(&c, 903000);
  TEST_CHECK(c.running == WHITE);
}

static void test_expiry(void) {
  Chess_clock_t c;
  clock_init(&c, 10000, 0);
  clock_start(&c, WHITE, 1000);

  /* NONE right up to the deadline, the running side from it on. */
  TEST_CHECK(clock_expired_side(&c, 1000) == NONE);
  TEST_CHECK(clock_expired_side(&c, 10999) == NONE);
  TEST_CHECK(clock_expired_side(&c, 11000) == WHITE);
  TEST_CHECK(clock_expired_side(&c, 60000) == WHITE);
  /* And it stays the running side, never the still one, however long. */
  TEST_CHECK(clock_expired_side(&c, 10000000) == WHITE);

  /* The still side cannot expire, even with nothing left. */
  clock_init(&c, 10000, 0);
  c.remaining_ms[BLACK] = 0;
  clock_start(&c, WHITE, 1000);
  TEST_CHECK(clock_expired_side(&c, 5000) == NONE);
  TEST_CHECK(clock_remaining(&c, BLACK, 5000) == 0);

  /* Nothing running: nothing expires. */
  clock_init(&c, 10000, 0);
  TEST_CHECK(clock_expired_side(&c, 10000000) == NONE);
}

static void check_format(int32_t ms, const char *expect) {
  char buf[CLOCK_FORMAT_MAX];
  clock_format(ms, buf, sizeof(buf));
  TEST_CHECK_MSG(strcmp(buf, expect) == 0, "clock_format(%d) = \"%s\", expected \"%s\"", ms,
                 buf, expect);
}

static void test_format(void) {
  static const struct {
    int32_t ms;
    const char *text;
  } TABLE[] = {
      /* Below ten seconds: tenths, truncated. */
      {0, "0.0"},
      {99, "0.0"},
      {100, "0.1"},
      {999, "0.9"},
      {1000, "1.0"},
      {1999, "1.9"},
      {9499, "9.4"},
      {9999, "9.9"},
      /* The ten-second boundary: m:ss from here up. */
      {10000, "0:10"},
      {10999, "0:10"},
      {59999, "0:59"},
      {60000, "1:00"},
      {587000, "9:47"},
      {600000, "10:00"},
      /* The hour boundary: h:mm:ss from here up. */
      {3599999, "59:59"},
      {3600000, "1:00:00"},
      {3900000, "1:05:00"},
      {7325000, "2:02:05"},
      /* Negative reads as zero rather than as a time. */
      {-1, "0.0"},
  };

  for (size_t i = 0; i < sizeof(TABLE) / sizeof(TABLE[0]); i++) {
    check_format(TABLE[i].ms, TABLE[i].text);
  }

  /* Never rounded up: no non-zero remainder ever formats as "0:00", so a
   * side with time left is never shown as having none. Swept across every
   * millisecond of the first two seconds and every second of the first two
   * hours, which covers both boundaries either side. */
  char buf[CLOCK_FORMAT_MAX];
  int bad = 0;
  for (int32_t ms = 1; ms < 2000; ms++) {
    clock_format(ms, buf, sizeof(buf));
    if (strcmp(buf, "0:00") == 0 || strcmp(buf, "0:00:00") == 0) {
      bad++;
    }
  }
  for (int32_t ms = 1; ms < 2 * 3600 * 1000; ms += 997) {
    clock_format(ms, buf, sizeof(buf));
    if (strcmp(buf, "0:00") == 0 || strcmp(buf, "0:00:00") == 0) {
      bad++;
    }
  }
  TEST_CHECK_MSG(bad == 0, "%d non-zero remainders formatted as zero", bad);

  /* Truncation, not rounding, at every boundary: one millisecond short of a
   * second still reads as the lower second. */
  check_format(1999, "1.9");
  check_format(119999, "1:59");
  check_format(3659999, "1:00:59");
}

void test_clock(void) {
  test_init_and_is_timed();
  test_remaining_is_computed();
  test_press();
  test_hold_and_resume();
  test_expiry();
  test_format();
}
