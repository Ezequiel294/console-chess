#include "app/newgame.h"

#include "app/prompt.h"
#include "core/chessclock.h"
#include "ui/render.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define C_TITLE 250
#define C_LABEL 246
#define C_VALUE 252
#define C_DIM 240

/* An option's value in milliseconds, or this, meaning "ask for one". */
#define OPTION_CUSTOM (-1)

typedef struct {
  const char *label;
  int32_t ms;
} Option_t;

/* Untimed is a time of zero and nothing else, which is exactly what
 * clock_init reads as an untimed game. */
static const Option_t TIME_OPTIONS[] = {
    {"Untimed", 0},          {"1 min", 60000},        {"3 min", 180000},
    {"5 min", 300000},       {"10 min", 600000},      {"15 min", 900000},
    {"30 min", 1800000},     {"60 min", 3600000},     {"Custom…", OPTION_CUSTOM},
};
#define TIME_OPTION_COUNT (int)(sizeof(TIME_OPTIONS) / sizeof(TIME_OPTIONS[0]))
#define TIME_DEFAULT_INDEX 4 /* 10 min */

static const Option_t INCREMENT_OPTIONS[] = {
    {"0 s", 0},      {"1 s", 1000},   {"2 s", 2000},  {"3 s", 3000},
    {"5 s", 5000},   {"10 s", 10000}, {"30 s", 30000}, {"Custom…", OPTION_CUSTOM},
};
#define INCREMENT_OPTION_COUNT (int)(sizeof(INCREMENT_OPTIONS) / sizeof(INCREMENT_OPTIONS[0]))
#define INCREMENT_DEFAULT_INDEX 0 /* 0 s */

/* What a custom entry may be, in the units the prompt asks for. The upper
 * bounds are there so a mistyped value is refused with an explanation rather
 * than starting a game nobody meant. */
#define CUSTOM_TIME_MIN_MINUTES 1
#define CUSTOM_TIME_MAX_MINUTES 1440
#define CUSTOM_INCREMENT_MAX_SECONDS 600

typedef enum { ROW_TIME, ROW_INCREMENT, ROW_START, ROW_COUNT } Row_id_t;

typedef struct NewGame_s NewGame_t;

/* A row is a label, its options, which one is current, and whether it
 * currently applies. The action row carries no options. */
typedef struct {
  const char *label;
  const Option_t *options; /* NULL for the action row */
  int option_count;
  int index;
  /* The value entered for the Custom… option, and whether one has been. Kept
   * beside the index rather than written into the options, which are shared
   * and constant. */
  int32_t custom_ms;
  int has_custom;
  /* Whether this row can currently be changed. A row that cannot is drawn
   * dimmed rather than removed, so the screen does not change shape as it is
   * used. */
  int (*enabled)(const NewGame_t *ng);
} Row_t;

struct NewGame_s {
  Row_t rows[ROW_COUNT];
  int selected;
  int row_y[ROW_COUNT]; /* where each row was last drawn, for a click */

  /* The row a Custom… prompt is open for, and the option index to fall back
   * to if it is cancelled or rejected — so a refused entry leaves the
   * settings exactly as they were. */
  int pending_row;
  int pending_restore_index;

  Cmd_t (*on_start)(void *ctx, Chess_clock_t clock);
  void *ctx;
  char message[96];
};

static NewGame_t g_newgame;

/* --- Row values ---------------------------------------------------------- */

static int always_enabled(const NewGame_t *ng) {
  (void)ng;
  return 1;
}

static int32_t row_value_ms(const Row_t *row) {
  if (row->options == NULL) {
    return 0;
  }
  int32_t ms = row->options[row->index].ms;
  return (ms == OPTION_CUSTOM) ? row->custom_ms : ms;
}

/* The increment applies only to a game that has a clock. */
static int increment_enabled(const NewGame_t *ng) {
  return row_value_ms(&ng->rows[ROW_TIME]) > 0;
}

/* What a row currently reads. A Custom… option that has been answered shows
 * the value entered rather than the word, so the row always states what the
 * game will actually be played with. */
static void row_value_text(const Row_t *row, Row_id_t id, char *out, size_t len) {
  if (row->options == NULL) {
    out[0] = '\0';
    return;
  }
  const Option_t *opt = &row->options[row->index];
  if (opt->ms != OPTION_CUSTOM || !row->has_custom) {
    snprintf(out, len, "%s", opt->label);
    return;
  }
  if (id == ROW_TIME) {
    snprintf(out, len, "%d min", row->custom_ms / 60000);
  } else {
    snprintf(out, len, "%d s", row->custom_ms / 1000);
  }
}

/* --- The Custom… prompt --------------------------------------------------- */

/* A whole number in the units the prompt named, and nothing else: no sign, no
 * decimal point, no units typed alongside. Anything else is not an amount of
 * time and is refused with an explanation rather than read as something. */
static int parse_whole(const char *text, long *out) {
  const char *p = text;
  while (*p == ' ') {
    p++;
  }
  if (*p == '\0') {
    return 0;
  }
  long value = 0;
  int digits = 0;
  for (; *p >= '0' && *p <= '9'; p++) {
    value = value * 10 + (*p - '0');
    digits++;
    if (value > 1000000) {
      return 0;
    }
  }
  if (digits == 0) {
    return 0;
  }
  while (*p == ' ') {
    p++;
  }
  if (*p != '\0') {
    return 0;
  }
  *out = value;
  return 1;
}

static int validate_time(const char *text, char *err, size_t err_len) {
  long minutes;
  if (!parse_whole(text, &minutes)) {
    snprintf(err, err_len, "Enter a whole number of minutes, e.g. 7.");
    return 0;
  }
  if (minutes < CUSTOM_TIME_MIN_MINUTES || minutes > CUSTOM_TIME_MAX_MINUTES) {
    snprintf(err, err_len, "Between %d and %d minutes.", CUSTOM_TIME_MIN_MINUTES,
             CUSTOM_TIME_MAX_MINUTES);
    return 0;
  }
  return 1;
}

static int validate_increment(const char *text, char *err, size_t err_len) {
  long seconds;
  if (!parse_whole(text, &seconds)) {
    snprintf(err, err_len, "Enter a whole number of seconds, e.g. 5.");
    return 0;
  }
  if (seconds > CUSTOM_INCREMENT_MAX_SECONDS) {
    snprintf(err, err_len, "At most %d seconds.", CUSTOM_INCREMENT_MAX_SECONDS);
    return 0;
  }
  return 1;
}

static Cmd_t on_custom_submit(void *ctx, const char *text) {
  NewGame_t *ng = (NewGame_t *)ctx;
  Row_t *row = &ng->rows[ng->pending_row];
  long value;
  if (!parse_whole(text, &value)) {
    /* prompt_screen validated this already; the guard is here so a change to
     * one can never silently outlive the other. */
    row->index = ng->pending_restore_index;
    return (Cmd_t){CMD_POP, NULL};
  }
  row->custom_ms = (ng->pending_row == ROW_TIME) ? (int32_t)(value * 60000)
                                                  : (int32_t)(value * 1000);
  row->has_custom = 1;
  ng->message[0] = '\0';
  return (Cmd_t){CMD_POP, NULL};
}

/* Cancelling leaves the settings untouched: the row goes back to whatever it
 * read before Custom… was reached. */
static Cmd_t on_custom_cancel(void *ctx) {
  NewGame_t *ng = (NewGame_t *)ctx;
  ng->rows[ng->pending_row].index = ng->pending_restore_index;
  return (Cmd_t){CMD_POP, NULL};
}

static Cmd_t open_custom_prompt(NewGame_t *ng, int row_id, int restore_index) {
  ng->pending_row = row_id;
  ng->pending_restore_index = restore_index;
  ng->message[0] = '\0';
  if (row_id == ROW_TIME) {
    return (Cmd_t){CMD_PUSH, prompt_screen("Time in minutes:", "", 4, validate_time,
                                            on_custom_submit, on_custom_cancel, ng)};
  }
  return (Cmd_t){CMD_PUSH, prompt_screen("Increment in seconds:", "", 3, validate_increment,
                                          on_custom_submit, on_custom_cancel, ng)};
}

/* --- Driving it ----------------------------------------------------------- */

/* Left and right change the highlighted row's value and nothing else — never
 * the highlight, and never the game. Reaching Custom… asks for the value it
 * stands for, which is a change to that setting rather than an action on the
 * screen: Enter stays the one thing that starts a game. */
static Cmd_t change_value(NewGame_t *ng, int delta) {
  Row_t *row = &ng->rows[ng->selected];
  if (row->options == NULL || !row->enabled(ng)) {
    return CMD_STAY;
  }
  int previous = row->index;
  row->index = (row->index + delta + row->option_count) % row->option_count;
  if (row->options[row->index].ms == OPTION_CUSTOM) {
    return open_custom_prompt(ng, ng->selected, previous);
  }
  ng->message[0] = '\0';
  return CMD_STAY;
}

static Cmd_t start_game(NewGame_t *ng) {
  int32_t initial = row_value_ms(&ng->rows[ROW_TIME]);
  int32_t increment = increment_enabled(ng) ? row_value_ms(&ng->rows[ROW_INCREMENT]) : 0;

  Chess_clock_t clock;
  clock_init(&clock, initial, increment);
  return ng->on_start(ng->ctx, clock);
}

static Cmd_t newgame_handle(void *ctx, const Event_t *ev) {
  NewGame_t *ng = (NewGame_t *)ctx;

  /* A click only moves the highlight, the same as an arrow key — it chooses
   * nothing and changes nothing, so a stray click can neither alter a
   * setting nor start a game. */
  if (ev->type == EV_MOUSE && ev->mouse.kind == MOUSE_PRESS && ev->mouse.button == 0) {
    for (int k = 0; k < ROW_COUNT; k++) {
      if (ev->mouse.row == ng->row_y[k]) {
        ng->selected = k;
        break;
      }
    }
    return CMD_STAY;
  }
  if (ev->type != EV_KEY) {
    return CMD_STAY;
  }

  switch (ev->key.name) {
  case KEY_ESCAPE:
    /* Nothing is started, nothing is ended, nothing is discarded. */
    return (Cmd_t){CMD_POP, NULL};
  case KEY_UP:
    ng->selected = (ng->selected - 1 + ROW_COUNT) % ROW_COUNT;
    return CMD_STAY;
  case KEY_DOWN:
    ng->selected = (ng->selected + 1) % ROW_COUNT;
    return CMD_STAY;
  case KEY_LEFT:
    return change_value(ng, -1);
  case KEY_RIGHT:
    return change_value(ng, 1);
  case KEY_ENTER:
    /* Only the action row acts. Enter on a setting row does nothing, so
     * confirming a value can never be confused with confirming the game. */
    if (ng->selected == ROW_START) {
      return start_game(ng);
    }
    return CMD_STAY;
  default:
    return CMD_STAY;
  }
}

/* --- Drawing -------------------------------------------------------------- */

#define LABEL_W 12

static void newgame_render(void *ctx, Rect r) {
  NewGame_t *ng = (NewGame_t *)ctx;

  draw_fill(r, ' ', COLOR_DEFAULT, COLOR_DEFAULT, ATTR_NONE);
  draw_text(r, 1, 0, "New Game", C_TITLE, COLOR_DEFAULT, ATTR_BOLD);

  int top = 3;
  for (int k = 0; k < ROW_COUNT; k++) {
    Row_t *row = &ng->rows[k];
    int y = top + k + (k == ROW_START ? 1 : 0); /* a blank row above the action */
    ng->row_y[k] = y;

    int enabled = row->enabled(ng);
    int fg = enabled ? C_LABEL : C_DIM;
    uint8_t attr = (k == ng->selected) ? ATTR_REVERSE : (enabled ? ATTR_NONE : ATTR_DIM);

    if (row->options == NULL) {
      draw_text(r, 3, y, row->label, enabled ? C_VALUE : C_DIM, COLOR_DEFAULT, attr);
      continue;
    }

    char value[32];
    row_value_text(row, (Row_id_t)k, value, sizeof(value));
    char line[64];
    /* The arrows say the value is what left and right move, and they are
     * there whether or not the row is highlighted, so the gesture does not
     * have to be discovered one row at a time. */
    snprintf(line, sizeof(line), "%-*s  ‹ %-8s ›", LABEL_W, row->label, value);
    draw_text(r, 3, y, line, fg, COLOR_DEFAULT, attr);
  }

  if (ng->message[0] != '\0') {
    draw_text(r, 3, top + ROW_COUNT + 3, ng->message, C_LABEL, COLOR_DEFAULT, ATTR_NONE);
  }

  /* The one hint row, where every screen puts how it is driven. */
  app_draw_bottom_hint(r, "↑/↓ select row  ·  ←/→ change value  ·  Enter start  ·  Esc back");
}

/* --- Construction --------------------------------------------------------- */

static void newgame_on_enter(void *ctx) {
  NewGame_t *ng = (NewGame_t *)ctx;
  ng->selected = ROW_START;
  ng->message[0] = '\0';
}

Screen *newgame_screen(Cmd_t (*on_start)(void *ctx, Chess_clock_t clock), void *ctx) {
  static Screen screen;

  memset(&g_newgame, 0, sizeof(g_newgame));

  /* Every row on this screen is a setting that can actually be chosen. A
   * further one — Opponent: Human / Bot — is one more entry here plus one
   * more Row_id_t: change_value, start_game, newgame_handle and
   * newgame_render all read the array rather than naming rows, so none of
   * them moves, and no existing row's behaviour changes. */
  g_newgame.rows[ROW_TIME] = (Row_t){.label = "Time",
                                      .options = TIME_OPTIONS,
                                      .option_count = TIME_OPTION_COUNT,
                                      .index = TIME_DEFAULT_INDEX,
                                      .enabled = always_enabled};
  g_newgame.rows[ROW_INCREMENT] = (Row_t){.label = "Increment",
                                           .options = INCREMENT_OPTIONS,
                                           .option_count = INCREMENT_OPTION_COUNT,
                                           .index = INCREMENT_DEFAULT_INDEX,
                                           .enabled = increment_enabled};
  g_newgame.rows[ROW_START] = (Row_t){.label = "Start game", .enabled = always_enabled};

  g_newgame.selected = ROW_START;
  g_newgame.on_start = on_start;
  g_newgame.ctx = ctx;

  screen.on_enter = newgame_on_enter;
  screen.on_exit = NULL;
  screen.handle = newgame_handle;
  screen.render = newgame_render;
  screen.ctx = &g_newgame;
  screen.opaque = 1;
  screen.wake_in_ms = NULL;
  screen.tick = NULL;
  return &screen;
}
