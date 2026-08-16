#include "ui/term.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/* ---------------------------------------------------------------------------
 * Teardown
 *
 * This half of the file is written and armed before any mode change is made.
 * A program that dies without restoring leaves the user in a shell with no
 * echo and no cursor, which reads as "the terminal went crazy" — the exact
 * complaint this module exists to prevent.
 * ------------------------------------------------------------------------ */

/* The exit sequence, in one piece:
 *   ?1000/1002/1003/1006l  all mouse reporting off
 *   ?2004l                 bracketed paste off
 *   ?1007h                 alternate scroll back on (it is the terminal's
 *                          default; we turned it off, so we turn it back)
 *   ?25h                   cursor visible
 *   ?1049l                 leave the alternate screen
 *   0m                     no lingering colour or attribute
 *
 * A string literal rather than something assembled at startup: it is built
 * once, lives in read-only memory, and needs neither formatting nor allocation
 * on the signal path, which is the property the signal handler requires.
 * Ordering matters only in that the alternate screen is left last, so nothing
 * above it is written to the restored screen. */
static const char RESTORE_SEQ[] =
    "\033[?1000l\033[?1002l\033[?1003l\033[?1006l"
    "\033[?2004l"
    "\033[?1007h"
    "\033[?25h"
    "\033[0m"
    "\033[?1049l";

static struct termios g_saved_termios;
static volatile sig_atomic_t g_have_saved_termios = 0;
static volatile sig_atomic_t g_entered = 0;
static volatile sig_atomic_t g_resized = 0;

/* Async-signal-safe: write(2) and tcsetattr(3) only, no stdio, no allocation.
 * Clearing g_entered first means a second call — atexit after the handler, or
 * two signals racing — writes nothing rather than fighting the first. */
void term_restore(void) {
  if (g_entered) {
    g_entered = 0;
    ssize_t ignored = write(STDOUT_FILENO, RESTORE_SEQ, sizeof(RESTORE_SEQ) - 1);
    (void)ignored;
  }
  if (g_have_saved_termios) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_saved_termios);
  }
}

/* Restore, then die the way the signal says to. Re-raising with the default
 * disposition preserves the exit status and the core dump: a SIGSEGV still
 * reports as a segmentation fault, it just no longer takes the shell with it. */
static void fatal_signal_handler(int sig) {
  term_restore();
  signal(sig, SIG_DFL);
  raise(sig);
}

static void winch_handler(int sig) {
  (void)sig;
  g_resized = 1;
}

/* ---------------------------------------------------------------------------
 * Setup
 * ------------------------------------------------------------------------ */

int term_is_interactive(void) {
  return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
}

int term_supports_color(void) {
  const char *no_color = getenv("NO_COLOR");
  if (no_color != NULL && *no_color != '\0') {
    return 0;
  }
  const char *term = getenv("TERM");
  if (term == NULL || *term == '\0' || strcmp(term, "dumb") == 0) {
    return 0;
  }
  return 1;
}

int term_init(void) {
  if (tcgetattr(STDIN_FILENO, &g_saved_termios) != 0) {
    return 0;
  }
  g_have_saved_termios = 1;

  atexit(term_restore);

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = fatal_signal_handler;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGSEGV, &sa, NULL);

  /* No SA_RESTART, deliberately: a resize must make a blocked read() return
   * EINTR so the loop redraws immediately, instead of the new size sitting
   * unnoticed until the player happens to press a key. */
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = winch_handler;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGWINCH, &sa, NULL);

  return 1;
}

/* Raw mode: no canonical line assembly, no echo, no input translation, and one
 * byte is enough to return from a read. ISIG stays on, so Ctrl-C still raises
 * SIGINT and still quits — a program that cannot be interrupted by reflex is
 * worse than one that loses a keybinding. */
static int set_raw_mode(void) {
  struct termios raw = g_saved_termios;
  raw.c_iflag &= (tcflag_t) ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  raw.c_oflag &= (tcflag_t) ~(OPOST);
  raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON | IEXTEN);
  raw.c_cflag |= (tcflag_t)CS8;
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  return tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0;
}

int term_enter(void) {
  if (!g_have_saved_termios) {
    return 0;
  }
  if (!set_raw_mode()) {
    return 0;
  }

  /* g_entered is set before the sequence is written, not after: if the write
   * is interrupted part-way the restore must still run. */
  g_entered = 1;

  static const char ENTER_SEQ[] =
      "\033[?1049h" /* alternate screen: the shell's scrollback is untouched */
      "\033[?25l"   /* hide the cursor; the game draws its own */
      /* Alternate scroll off. Do not remove this. With it on — and it is on by
       * default inside the alternate screen — the terminal translates wheel
       * motion into cursor-key sequences that are byte-for-byte identical to
       * the arrow keys. Every scroll then arrives as keyboard input that no
       * amount of parsing can tell from a real keypress. This one sequence is
       * the whole of that bug. */
      "\033[?1007l"
      /* Mouse reporting: ?1000h is click tracking (press and release, no
       * motion); ?1006h switches the report encoding to SGR, which — unlike
       * the legacy encoding it replaces — can express a column past 223.
       * RESTORE_SEQ above already turns both off, and 1002/1003 as well, so
       * the teardown path was ready for this before it was ever turned on. */
      "\033[?1000h"
      "\033[?1006h"
      "\033[?2004h" /* bracketed paste: a paste arrives framed, not as keys */
      "\033[2J";    /* start from a known-empty alternate screen */

  term_write(ENTER_SEQ, sizeof(ENTER_SEQ) - 1);
  return 1;
}

Term_size_t term_size(void) {
  struct winsize ws;
  Term_size_t size = {80, 24};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
    size.cols = ws.ws_col;
    size.rows = ws.ws_row;
  }
  return size;
}

int term_take_resize(void) {
  if (!g_resized) {
    return 0;
  }
  g_resized = 0;
  return 1;
}

/* ---------------------------------------------------------------------------
 * I/O
 * ------------------------------------------------------------------------ */

void term_write(const char *buf, unsigned long n) {
  unsigned long written = 0;
  while (written < n) {
    ssize_t w = write(STDOUT_FILENO, buf + written, n - written);
    if (w < 0) {
      if (errno == EINTR) {
        continue; /* a resize mid-write is not a reason to lose the frame */
      }
      return;
    }
    written += (unsigned long)w;
  }
}

long term_read(char *buf, unsigned long n, int timeout_ms) {
  if (timeout_ms >= 0) {
    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    pfd.revents = 0;
    int r = poll(&pfd, 1, timeout_ms);
    if (r < 0) {
      return (errno == EINTR) ? TERM_READ_INTR : TERM_READ_EOF;
    }
    if (r == 0) {
      return 0; /* timed out with nothing to read */
    }
  }

  ssize_t got = read(STDIN_FILENO, buf, n);
  if (got < 0) {
    return (errno == EINTR) ? TERM_READ_INTR : TERM_READ_EOF;
  }
  if (got == 0) {
    return TERM_READ_EOF;
  }
  return (long)got;
}

/* ---------------------------------------------------------------------------
 * Glyph width probe
 *
 * wcwidth() reports 1 for the Private Use Area codepoints the piece icons use,
 * while many terminals draw them across two cells. Assuming either value shears
 * every board row on half of all terminals, so the terminal is asked instead:
 * draw one glyph, ask where the cursor ended up, subtract.
 * ------------------------------------------------------------------------ */

/* Reads a cursor-position report, \033[<row>;<col>R, within budget_ms total.
 * Returns the column, or 0 if nothing usable arrived in time. Anything that is
 * not the report is skipped rather than pushed back: this runs before the input
 * pipeline exists, so there is no consumer for stray bytes and no buffer for
 * them to corrupt. */
static int read_cursor_column(int budget_ms) {
  char buf[32];
  size_t len = 0;
  int remaining = budget_ms;

  while (remaining > 0 && len < sizeof(buf) - 1) {
    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    pfd.revents = 0;

    int step = remaining < 20 ? remaining : 20;
    int r = poll(&pfd, 1, step);
    if (r < 0) {
      if (errno == EINTR) {
        continue;
      }
      return 0;
    }
    if (r == 0) {
      remaining -= step;
      continue;
    }

    ssize_t got = read(STDIN_FILENO, buf + len, sizeof(buf) - 1 - len);
    if (got <= 0) {
      return 0;
    }
    len += (size_t)got;
    buf[len] = '\0';

    const char *r_byte = memchr(buf, 'R', len);
    if (r_byte != NULL) {
      const char *semi = memchr(buf, ';', (size_t)(r_byte - buf));
      if (semi == NULL) {
        return 0;
      }
      return atoi(semi + 1);
    }
  }
  return 0;
}

int term_probe_glyph_width(const char *sample) {
  if (!g_have_saved_termios || sample == NULL || *sample == '\0') {
    return TERM_GLYPH_WIDTH_DEFAULT;
  }
  if (!set_raw_mode()) {
    return TERM_GLYPH_WIDTH_DEFAULT;
  }

  /* Column 1 of a fresh line, one glyph, then ask. Drawn on the primary screen
   * before term_enter, and erased below, so nothing of this reaches the game
   * display. */
  term_write("\r\033[K", 4);
  term_write(sample, strlen(sample));
  term_write("\033[6n", 4);

  /* Bounded: some terminals never answer, and an unbounded read here would
   * hang the program before it has drawn anything at all. */
  int column = read_cursor_column(100);

  term_write("\r\033[K", 4);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_saved_termios);

  int width = column - 1;
  if (width != 1 && width != 2) {
    return TERM_GLYPH_WIDTH_DEFAULT;
  }
  return width;
}
