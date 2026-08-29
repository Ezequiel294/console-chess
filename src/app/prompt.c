#include "app/prompt.h"

#include "ui/render.h"

#include <stdio.h>
#include <string.h>

#define C_BOX_BG 236
#define C_BOX_FG 250

/* A safety cap on how many codepoints this screen can ever be asked to hold,
 * independent of whatever max_codepoints a caller passes — large enough for
 * every current use (32, for a save name) with room to spare. */
#define PROMPT_MAX_CODEPOINTS 64
/* Four bytes per codepoint, worst case, plus a NUL. */
#define PROMPT_BUF_LEN (PROMPT_MAX_CODEPOINTS * 4 + 1)

typedef struct {
  char title[64];
  char err[96];

  char buf[PROMPT_BUF_LEN];
  int byte_len;
  /* Byte length of each codepoint currently in buf, in order, so backspace
   * can remove exactly one whole codepoint rather than guessing where the
   * previous one started. */
  int codepoint_bytes[PROMPT_MAX_CODEPOINTS];
  int codepoint_count;
  int max_codepoints;

  int (*validate)(const char *text, char *err, size_t err_len);
  Cmd_t (*on_submit)(void *ctx, const char *text);
  Cmd_t (*on_cancel)(void *ctx);
  void *ctx;
} Prompt_t;

static Prompt_t g_prompt;

static void encode_utf8(uint32_t cp, char out[4], int *n) {
  if (cp < 0x80u) {
    out[0] = (char)cp;
    *n = 1;
  } else if (cp < 0x800u) {
    out[0] = (char)(0xC0u | (cp >> 6));
    out[1] = (char)(0x80u | (cp & 0x3Fu));
    *n = 2;
  } else if (cp < 0x10000u) {
    out[0] = (char)(0xE0u | (cp >> 12));
    out[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
    out[2] = (char)(0x80u | (cp & 0x3Fu));
    *n = 3;
  } else {
    out[0] = (char)(0xF0u | (cp >> 18));
    out[1] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
    out[2] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
    out[3] = (char)(0x80u | (cp & 0x3Fu));
    *n = 4;
  }
}

/* Decodes one UTF-8 codepoint from s, for seeding the field from
 * initial_value. Returns the bytes consumed, or 0 at the end of the string.
 * Malformed input consumes one byte and yields U+FFFD rather than stalling —
 * this only ever reads a caller-supplied name, never terminal input. */
static int decode_utf8(const char *s, uint32_t *out) {
  const unsigned char *p = (const unsigned char *)s;
  if (p[0] == 0) {
    return 0;
  }
  if (p[0] < 0x80u) {
    *out = p[0];
    return 1;
  }
  int len;
  uint32_t cp;
  if ((p[0] & 0xE0u) == 0xC0u) {
    len = 2;
    cp = p[0] & 0x1Fu;
  } else if ((p[0] & 0xF0u) == 0xE0u) {
    len = 3;
    cp = p[0] & 0x0Fu;
  } else if ((p[0] & 0xF8u) == 0xF0u) {
    len = 4;
    cp = p[0] & 0x07u;
  } else {
    *out = 0xFFFDu;
    return 1;
  }
  for (int i = 1; i < len; i++) {
    if ((p[i] & 0xC0u) != 0x80u) {
      *out = 0xFFFDu;
      return 1;
    }
    cp = (cp << 6) | (p[i] & 0x3Fu);
  }
  *out = cp;
  return len;
}

static void append_codepoint(Prompt_t *p, uint32_t cp) {
  if (p->codepoint_count >= p->max_codepoints || p->codepoint_count >= PROMPT_MAX_CODEPOINTS) {
    return;
  }
  char enc[4];
  int n;
  encode_utf8(cp, enc, &n);
  if (p->byte_len + n >= (int)sizeof(p->buf)) {
    return;
  }
  memcpy(p->buf + p->byte_len, enc, (size_t)n);
  p->byte_len += n;
  p->buf[p->byte_len] = '\0';
  p->codepoint_bytes[p->codepoint_count] = n;
  p->codepoint_count++;
}

static void backspace_codepoint(Prompt_t *p) {
  if (p->codepoint_count == 0) {
    return;
  }
  p->codepoint_count--;
  p->byte_len -= p->codepoint_bytes[p->codepoint_count];
  p->buf[p->byte_len] = '\0';
}

static void prompt_render(void *ctx, Rect r) {
  Prompt_t *p = (Prompt_t *)ctx;

  int w = 46;
  if (w > r.w) {
    w = r.w;
  }
  int h = 6;
  if (h > r.h) {
    h = r.h;
  }
  int x = (r.w - w) / 2, y = (r.h - h) / 2;
  if (x < 0) {
    x = 0;
  }
  if (y < 0) {
    y = 0;
  }
  Rect box = rect_sub(r, x, y, w, h);

  draw_fill(box, ' ', COLOR_DEFAULT, C_BOX_BG, ATTR_NONE);
  draw_box(box, C_BOX_FG, C_BOX_BG, ATTR_NONE);

  Rect inner = rect_inset(box, 2, 1);
  draw_text(inner, 0, 0, p->title, C_BOX_FG, C_BOX_BG, ATTR_BOLD);

  char field[PROMPT_BUF_LEN + 1];
  snprintf(field, sizeof(field), "%s_", p->buf);
  draw_text(inner, 0, 2, field, COLOR_DEFAULT, C_BOX_BG, ATTR_NONE);

  if (p->err[0] != '\0') {
    draw_text(inner, 0, 3, p->err, C_BOX_FG, C_BOX_BG, ATTR_DIM);
  }

  app_draw_bottom_hint(r, "Enter confirm  ·  Esc cancel");
}

static Cmd_t prompt_handle(void *ctx, const Event_t *ev) {
  Prompt_t *p = (Prompt_t *)ctx;

  if (ev->type != EV_KEY) {
    return CMD_STAY;
  }
  if (ev->key.name == KEY_ESCAPE) {
    return p->on_cancel != NULL ? p->on_cancel(p->ctx) : (Cmd_t){CMD_POP, NULL};
  }
  if (ev->key.name == KEY_BACKSPACE) {
    backspace_codepoint(p);
    p->err[0] = '\0';
    return CMD_STAY;
  }
  if (ev->key.name == KEY_ENTER) {
    if (p->validate != NULL && !p->validate(p->buf, p->err, sizeof(p->err))) {
      return CMD_STAY; /* rejected: stays open, err shown, text untouched */
    }
    return p->on_submit(p->ctx, p->buf);
  }
  if (ev->key.name == KEY_CHAR && ev->key.ch >= 0x20 && ev->key.ch != 0x7F) {
    append_codepoint(p, ev->key.ch);
    p->err[0] = '\0';
    return CMD_STAY;
  }
  return CMD_STAY;
}

Screen *prompt_screen(const char *title, const char *initial_value, int max_codepoints,
                       int (*validate)(const char *text, char *err, size_t err_len),
                       Cmd_t (*on_submit)(void *ctx, const char *text),
                       Cmd_t (*on_cancel)(void *ctx), void *ctx) {
  static Screen screen;

  memset(&g_prompt, 0, sizeof(g_prompt));
  snprintf(g_prompt.title, sizeof(g_prompt.title), "%s", title);
  g_prompt.max_codepoints =
      (max_codepoints < PROMPT_MAX_CODEPOINTS) ? max_codepoints : PROMPT_MAX_CODEPOINTS;
  g_prompt.validate = validate;
  g_prompt.on_submit = on_submit;
  g_prompt.on_cancel = on_cancel;
  g_prompt.ctx = ctx;

  if (initial_value != NULL) {
    const char *s = initial_value;
    uint32_t cp;
    int n;
    while ((n = decode_utf8(s, &cp)) > 0 && g_prompt.codepoint_count < g_prompt.max_codepoints) {
      append_codepoint(&g_prompt, cp);
      s += n;
    }
  }

  screen.on_enter = NULL;
  screen.on_exit = NULL;
  screen.handle = prompt_handle;
  screen.render = prompt_render;
  screen.ctx = &g_prompt;
  screen.opaque = 0;
  return &screen;
}
