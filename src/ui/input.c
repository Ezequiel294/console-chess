#include "ui/input.h"

#include "ui/term.h"

#include <stdlib.h>
#include <string.h>

/* Unread bytes. Held across calls, which is what lets a sequence interrupted by
 * a resize be resumed rather than restarted: the interruption returns to the
 * caller with the bytes still here, and the next call carries on from where the
 * parse stopped. */
static char g_buf[4096];
static size_t g_len = 0;

/* Pasted text, accumulated across as many reads as it takes. */
static char *g_paste = NULL;
static size_t g_paste_len = 0;
static size_t g_paste_cap = 0;
static int g_in_paste = 0;

/* How long to wait for the rest of something already begun.
 *
 * A bare Escape and the first byte of an escape sequence are the same byte, so
 * the only way to tell them apart is to wait and see whether more follows.
 * 25 ms is far longer than a terminal takes to deliver the rest of a sequence
 * and far shorter than a person notices. */
#define ESC_TIMEOUT_MS 25
/* An escape sequence that has started but not finished. Terminals do not send
 * these in pieces, so reaching this timeout means the sequence was truncated;
 * it is dropped rather than emitted as keys. */
#define PARTIAL_TIMEOUT_MS 250
/* A paste body, which legitimately arrives over many reads and can be large. */
#define PASTE_TIMEOUT_MS 2000

#define ESC 0x1B

typedef enum {
  DECODE_EVENT,     /* an event was produced */
  DECODE_CONSUMED,  /* bytes were consumed, no event: go round again */
  DECODE_NEED_MORE  /* an incomplete sequence: read more bytes */
} Decode_result_t;

/* --- Buffers ------------------------------------------------------------ */

static void consume(size_t n) {
  if (n >= g_len) {
    g_len = 0;
    return;
  }
  memmove(g_buf, g_buf + n, g_len - n);
  g_len -= n;
}

static void paste_append(const char *s, size_t n) {
  if (g_paste_len + n + 1 > g_paste_cap) {
    size_t cap = g_paste_cap ? g_paste_cap : 256;
    while (cap < g_paste_len + n + 1) {
      cap *= 2;
    }
    char *grown = realloc(g_paste, cap);
    if (grown == NULL) {
      return; /* the paste is truncated; it is text, not state */
    }
    g_paste = grown;
    g_paste_cap = cap;
  }
  memcpy(g_paste + g_paste_len, s, n);
  g_paste_len += n;
  g_paste[g_paste_len] = '\0';
}

void input_shutdown(void) {
  free(g_paste);
  g_paste = NULL;
  g_paste_len = 0;
  g_paste_cap = 0;
  g_len = 0;
  g_in_paste = 0;
}

/* --- Event construction ------------------------------------------------- */

static Event_t make_key(Key_name_t name, uint32_t ch, int alt) {
  Event_t ev;
  memset(&ev, 0, sizeof(ev));
  ev.type = EV_KEY;
  ev.key.name = name;
  ev.key.ch = ch;
  ev.key.alt = alt;
  return ev;
}

/* --- UTF-8 -------------------------------------------------------------- */

/* Bytes in the character starting with b, or 1 for anything that is not a
 * lead byte — a stray continuation byte is consumed rather than allowed to
 * stall the parse. */
static int utf8_len(unsigned char b) {
  if (b < 0x80u) {
    return 1;
  }
  if ((b & 0xE0u) == 0xC0u) {
    return 2;
  }
  if ((b & 0xF0u) == 0xE0u) {
    return 3;
  }
  if ((b & 0xF8u) == 0xF0u) {
    return 4;
  }
  return 1;
}

static uint32_t utf8_decode(const char *s, int len) {
  const unsigned char *p = (const unsigned char *)s;
  if (len == 1) {
    return p[0];
  }
  uint32_t cp = (uint32_t)(p[0] & (0xFFu >> (len + 1)));
  for (int i = 1; i < len; i++) {
    cp = (cp << 6) | (p[i] & 0x3Fu);
  }
  return cp;
}

/* --- The parser --------------------------------------------------------- */

/* A byte that is not part of an escape sequence. Control codes that name a key
 * become that key; the rest — Ctrl-L and friends — arrive as their control
 * codepoint, so a screen can bind them. */
static Decode_result_t decode_plain(Event_t *out, int alt, size_t offset) {
  unsigned char b = (unsigned char)g_buf[offset];

  switch (b) {
  case '\r':
  case '\n':
    *out = make_key(KEY_ENTER, 0, alt);
    consume(offset + 1);
    return DECODE_EVENT;
  case '\t':
    *out = make_key(KEY_TAB, 0, alt);
    consume(offset + 1);
    return DECODE_EVENT;
  case 0x08:
  case 0x7F:
    *out = make_key(KEY_BACKSPACE, 0, alt);
    consume(offset + 1);
    return DECODE_EVENT;
  default:
    break;
  }

  int len = utf8_len(b);
  if (offset + (size_t)len > g_len) {
    return DECODE_NEED_MORE;
  }
  *out = make_key(KEY_CHAR, utf8_decode(g_buf + offset, len), alt);
  consume(offset + (size_t)len);
  return DECODE_EVENT;
}

/* \033[<b;col;rowM or ...m — SGR mouse reporting, used rather than the older
 * encoding because that one cannot express a column past 223. */
static Decode_result_t decode_mouse(Event_t *out, const char *params, size_t plen,
                                    char final, size_t total) {
  int b = 0;
  int col = 0;
  int row = 0;
  int field = 0;
  int *fields[3] = {&b, &col, &row};

  for (size_t i = 1; i < plen; i++) { /* skip the leading '<' */
    char c = params[i];
    if (c >= '0' && c <= '9') {
      if (*fields[field] < 100000) { /* clamped; see decode_csi */
        *fields[field] = *fields[field] * 10 + (c - '0');
      }
    } else if (c == ';' && field < 2) {
      field++;
    } else {
      consume(total);
      return DECODE_CONSUMED; /* malformed: consumed whole, reported as nothing */
    }
  }
  if (field != 2) {
    consume(total);
    return DECODE_CONSUMED;
  }

  memset(out, 0, sizeof(*out));
  out->type = EV_MOUSE;
  /* Terminals count from 1; everything else in this program counts from 0. */
  out->mouse.col = col - 1;
  out->mouse.row = row - 1;

  if (b & 64) {
    /* Buttons 64 and 65 are the wheel. Not recognising them is how wheel
     * reports used to fall through the parser and surface as text. */
    out->mouse.kind = MOUSE_WHEEL;
    out->mouse.wheel = (b & 1) ? -1 : 1;
    out->mouse.button = -1;
  } else if (b & 32) {
    out->mouse.kind = MOUSE_MOTION;
    out->mouse.button = b & 3;
  } else {
    out->mouse.kind = (final == 'm') ? MOUSE_RELEASE : MOUSE_PRESS;
    out->mouse.button = b & 3;
  }

  consume(total);
  return DECODE_EVENT;
}

static Key_name_t tilde_key(int param) {
  switch (param) {
  case 1:
  case 7:
    return KEY_HOME;
  case 2:
    return KEY_INSERT;
  case 3:
    return KEY_DELETE;
  case 4:
  case 8:
    return KEY_END;
  case 5:
    return KEY_PAGE_UP;
  case 6:
    return KEY_PAGE_DOWN;
  case 11:
    return KEY_F1;
  case 12:
    return KEY_F2;
  case 13:
    return KEY_F3;
  case 14:
    return KEY_F4;
  case 15:
    return KEY_F5;
  case 17:
    return KEY_F6;
  case 18:
    return KEY_F7;
  case 19:
    return KEY_F8;
  case 20:
    return KEY_F9;
  case 21:
    return KEY_F10;
  case 23:
    return KEY_F11;
  case 24:
    return KEY_F12;
  default:
    return KEY_CHAR; /* stands for "not a key we name"; the caller discards */
  }
}

/* \033[ ... final, where final is 0x40-0x7E. The sequence is always consumed
 * through that final byte, whether or not it is recognised: a parser that gives
 * up part-way leaves the tail in the stream to be misread as keystrokes. */
static Decode_result_t decode_csi(Event_t *out) {
  size_t i = 2;

  while (i < g_len && (unsigned char)g_buf[i] >= 0x30 && (unsigned char)g_buf[i] <= 0x3F) {
    i++;
  }
  size_t params_end = i;
  while (i < g_len && (unsigned char)g_buf[i] >= 0x20 && (unsigned char)g_buf[i] <= 0x2F) {
    i++;
  }
  if (i >= g_len) {
    return DECODE_NEED_MORE;
  }

  unsigned char final = (unsigned char)g_buf[i];
  if (final < 0x40 || final > 0x7E) {
    /* Not a valid final byte: the sequence was aborted. Drop what has been read
     * of it and let this byte be parsed on its own terms. */
    consume(i);
    return DECODE_CONSUMED;
  }

  const char *params = g_buf + 2;
  size_t plen = params_end - 2;
  size_t total = i + 1;

  if (plen > 0 && params[0] == '<' && (final == 'M' || final == 'm')) {
    return decode_mouse(out, params, plen, (char)final, total);
  }

  /* Anything with a private-parameter prefix other than the mouse report above
   * is a terminal talking to itself. */
  if (plen > 0 && (params[0] == '<' || params[0] == '=' || params[0] == '>' || params[0] == '?')) {
    consume(total);
    return DECODE_CONSUMED;
  }

  /* A leading numeric parameter; modifier parameters after it are ignored, so
   * Shift-Left is still Left rather than nothing. */
  int param = 0;
  int have_param = 0;
  for (size_t k = 0; k < plen; k++) {
    if (params[k] >= '0' && params[k] <= '9') {
      /* Clamped: a terminal is free to send a parameter of any length, and no
       * key this maps to is anywhere near the limit. */
      if (param < 100000) {
        param = param * 10 + (params[k] - '0');
      }
      have_param = 1;
    } else {
      break;
    }
  }

  Key_name_t name;
  switch (final) {
  case 'A':
    name = KEY_UP;
    break;
  case 'B':
    name = KEY_DOWN;
    break;
  case 'C':
    name = KEY_RIGHT;
    break;
  case 'D':
    name = KEY_LEFT;
    break;
  case 'H':
    name = KEY_HOME;
    break;
  case 'F':
    name = KEY_END;
    break;
  case '~':
    if (have_param && param == 200) {
      /* Paste start: the body follows, and is gathered whole. */
      consume(total);
      g_in_paste = 1;
      g_paste_len = 0;
      if (g_paste != NULL) {
        g_paste[0] = '\0';
      }
      return DECODE_CONSUMED;
    }
    if (!have_param) {
      consume(total);
      return DECODE_CONSUMED;
    }
    name = tilde_key(param);
    if (name == KEY_CHAR) {
      consume(total);
      return DECODE_CONSUMED;
    }
    break;
  default:
    consume(total);
    return DECODE_CONSUMED;
  }

  *out = make_key(name, 0, 0);
  consume(total);
  return DECODE_EVENT;
}

/* The string sequences — OSC, DCS, SOS, PM, APC — which carry a payload of
 * arbitrary text and end at a string terminator rather than at a final byte.
 * A terminal answering a colour or title query sends one of these unprompted,
 * and its payload is ordinary printable text: parsed as anything but a unit, it
 * arrives as a burst of keystrokes. */
static Decode_result_t decode_string_seq(void) {
  for (size_t i = 2; i < g_len; i++) {
    unsigned char b = (unsigned char)g_buf[i];
    if (b == 0x07) { /* BEL, which OSC accepts as a terminator */
      consume(i + 1);
      return DECODE_CONSUMED;
    }
    if (b == ESC) {
      if (i + 1 >= g_len) {
        return DECODE_NEED_MORE;
      }
      if (g_buf[i + 1] == '\\') { /* ST */
        consume(i + 2);
        return DECODE_CONSUMED;
      }
      /* An escape that is not a terminator abandons the string. Drop what has
       * been read and reconsider from that escape. */
      consume(i);
      return DECODE_CONSUMED;
    }
  }
  return DECODE_NEED_MORE;
}

/* \033O plus one byte: the function keys on terminals in application mode. */
static Decode_result_t decode_ss3(Event_t *out) {
  if (g_len < 3) {
    return DECODE_NEED_MORE;
  }
  Key_name_t name;
  switch (g_buf[2]) {
  case 'P':
    name = KEY_F1;
    break;
  case 'Q':
    name = KEY_F2;
    break;
  case 'R':
    name = KEY_F3;
    break;
  case 'S':
    name = KEY_F4;
    break;
  case 'A':
    name = KEY_UP;
    break;
  case 'B':
    name = KEY_DOWN;
    break;
  case 'C':
    name = KEY_RIGHT;
    break;
  case 'D':
    name = KEY_LEFT;
    break;
  case 'H':
    name = KEY_HOME;
    break;
  case 'F':
    name = KEY_END;
    break;
  default:
    consume(3);
    return DECODE_CONSUMED;
  }
  *out = make_key(name, 0, 0);
  consume(3);
  return DECODE_EVENT;
}

/* Gathers a paste body until its end marker. Everything before the marker is
 * text; nothing in it produces a key event. */
static Decode_result_t decode_paste_body(Event_t *out) {
  static const char END[] = "\033[201~";
  const size_t end_len = sizeof(END) - 1;

  for (size_t i = 0; i + end_len <= g_len; i++) {
    if (memcmp(g_buf + i, END, end_len) == 0) {
      paste_append(g_buf, i);
      consume(i + end_len);
      g_in_paste = 0;
      memset(out, 0, sizeof(*out));
      out->type = EV_PASTE;
      out->paste.text = g_paste != NULL ? g_paste : "";
      out->paste.len = g_paste_len;
      return DECODE_EVENT;
    }
  }

  /* Keep back as much as the end marker could be part of, so a marker split
   * across two reads is still found. */
  if (g_len > end_len - 1) {
    size_t take = g_len - (end_len - 1);
    paste_append(g_buf, take);
    consume(take);
  }
  return DECODE_NEED_MORE;
}

static Decode_result_t decode_one(Event_t *out) {
  if (g_in_paste) {
    return decode_paste_body(out);
  }
  if (g_len == 0) {
    return DECODE_NEED_MORE;
  }
  if ((unsigned char)g_buf[0] != ESC) {
    return decode_plain(out, 0, 0);
  }
  if (g_len == 1) {
    return DECODE_NEED_MORE; /* Escape, or the start of a sequence: wait and see */
  }

  switch (g_buf[1]) {
  case '[':
    return decode_csi(out);
  case 'O':
    return decode_ss3(out);
  case ']': /* OSC */
  case 'P': /* DCS */
  case 'X': /* SOS */
  case '^': /* PM  */
  case '_': /* APC */
    return decode_string_seq();
  case ESC:
    /* Two escapes: report the first and reconsider the second on its own. */
    *out = make_key(KEY_ESCAPE, 0, 0);
    consume(1);
    return DECODE_EVENT;
  default:
    return decode_plain(out, 1, 1);
  }
}

/* --- The loop ----------------------------------------------------------- */

static int pending_timeout(void) {
  if (g_in_paste) {
    return PASTE_TIMEOUT_MS;
  }
  if (g_len == 0) {
    return -1; /* nothing started: wait as long as it takes */
  }
  if ((unsigned char)g_buf[0] == ESC) {
    return g_len == 1 ? ESC_TIMEOUT_MS : PARTIAL_TIMEOUT_MS;
  }
  return -1; /* a partial character; the rest is already on its way */
}

Event_t input_next(void) {
  Event_t ev;

  for (;;) {
    if (term_take_resize()) {
      memset(&ev, 0, sizeof(ev));
      ev.type = EV_RESIZE;
      return ev;
    }

    Decode_result_t r = decode_one(&ev);
    if (r == DECODE_EVENT) {
      return ev;
    }
    if (r == DECODE_CONSUMED) {
      continue;
    }

    if (g_len == sizeof(g_buf)) {
      /* A full buffer that still does not parse is not going to. Dropping it
       * loses input; keeping it would deadlock. */
      g_len = 0;
      continue;
    }

    int timeout = pending_timeout();
    long n = term_read(g_buf + g_len, sizeof(g_buf) - g_len, timeout);

    if (n == TERM_READ_INTR) {
      /* A signal, almost certainly the resize. The buffer is untouched, so the
       * sequence in progress resumes on the next pass rather than restarting. */
      continue;
    }
    if (n == TERM_READ_EOF) {
      memset(&ev, 0, sizeof(ev));
      ev.type = EV_EOF;
      return ev;
    }
    if (n == 0) {
      /* Nothing arrived within the wait. */
      if (g_in_paste) {
        g_in_paste = 0;
        g_paste_len = 0;
        g_len = 0;
        continue;
      }
      if (g_len == 1 && (unsigned char)g_buf[0] == ESC) {
        consume(1);
        return make_key(KEY_ESCAPE, 0, 0);
      }
      g_len = 0; /* a truncated sequence: discarded, never emitted as keys */
      continue;
    }

    g_len += (size_t)n;
  }
}
