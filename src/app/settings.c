#include "app/settings.h"

#include "ui/glyphs.h"
#include "ui/render.h"
#include "ui/term.h"

#include <stdio.h>
#include <string.h>

#define SETTINGS_PATH "settings.txt"

static const Palette_t PALETTES[PALETTE_COUNT] = {
    [PALETTE_CLASSIC] = {.square_last = 22,
                          .square_selected = 58,
                          .square_capture = 52,
                          .square_check = 130,
                          .mark_check_fg = 208},
    [PALETTE_OCEAN] = {.square_last = 23,
                        .square_selected = 24,
                        .square_capture = 88,
                        .square_check = 25,
                        .mark_check_fg = 45},
};

static int g_ascii = 0;
static Palette_id_t g_palette = PALETTE_CLASSIC;
static int g_icon_width = TERM_GLYPH_WIDTH_DEFAULT;

static int write_settings(void) {
  FILE *f = fopen(SETTINGS_PATH, "w");
  if (f == NULL) {
    return 0;
  }
  fprintf(f, "ascii=%d\n", g_ascii);
  fprintf(f, "palette=%d\n", (int)g_palette);
  int ok = !ferror(f);
  if (fclose(f) != 0) {
    ok = 0;
  }
  return ok;
}

static void apply_ascii(void) {
  glyphs_use_ascii(g_ascii);
  if (!g_ascii) {
    glyphs_set_width(g_icon_width);
  }
}

void settings_set_icon_width(int width) {
  g_icon_width = width;
  if (!g_ascii) {
    glyphs_set_width(g_icon_width);
  }
}

void settings_load(void) {
  FILE *f = fopen(SETTINGS_PATH, "r");
  if (f == NULL) {
    return;
  }
  char line[64];
  while (fgets(line, sizeof(line), f) != NULL) {
    int value;
    if (sscanf(line, "ascii=%d", &value) == 1) {
      g_ascii = value ? 1 : 0;
    } else if (sscanf(line, "palette=%d", &value) == 1 && value >= 0 && value < PALETTE_COUNT) {
      g_palette = (Palette_id_t)value;
    }
  }
  fclose(f);
  apply_ascii();
}

int settings_ascii(void) { return g_ascii; }
Palette_id_t settings_palette_id(void) { return g_palette; }
const Palette_t *settings_palette(void) { return &PALETTES[g_palette]; }

void settings_set_ascii(int on, int *persist_failed) {
  g_ascii = on ? 1 : 0;
  apply_ascii();
  int ok = write_settings();
  if (persist_failed != NULL) {
    *persist_failed = !ok;
  }
}

void settings_set_palette_id(Palette_id_t id, int *persist_failed) {
  if (id < 0 || id >= PALETTE_COUNT) {
    return;
  }
  g_palette = id;
  int ok = write_settings();
  if (persist_failed != NULL) {
    *persist_failed = !ok;
  }
}

/* --- Screen --------------------------------------------------------------- */

#define C_BOX_BG 236
#define C_BOX_FG 250

#define SETTINGS_ITEM_COUNT 2

typedef struct {
  int selected;
  int row_y[SETTINGS_ITEM_COUNT]; /* where each row was last drawn, for a click */
  char message[96];
} Settings_ui_t;

static Settings_ui_t g_settings_ui;

static const char *palette_name(Palette_id_t id) {
  return (id == PALETTE_OCEAN) ? "Ocean" : "Classic";
}

static void note_persist(Settings_ui_t *ui, int failed) {
  if (failed) {
    snprintf(ui->message, sizeof(ui->message),
             "Applied, but could not be saved for next time.");
  } else {
    ui->message[0] = '\0';
  }
}

/* Activates item k: cycles its value. The one place "choosing" a row is
 * defined, so Enter always means the same thing regardless of how the row
 * became selected. */
static void activate(Settings_ui_t *ui, int k) {
  int failed = 0;
  if (k == 0) {
    settings_set_ascii(!g_ascii, &failed);
  } else if (k == 1) {
    settings_set_palette_id((g_palette + 1) % PALETTE_COUNT, &failed);
  }
  note_persist(ui, failed);
}

static void settings_on_enter(void *ctx) {
  Settings_ui_t *ui = (Settings_ui_t *)ctx;
  ui->selected = 0;
  ui->message[0] = '\0';
}

static void settings_render(void *ctx, Rect r) {
  Settings_ui_t *ui = (Settings_ui_t *)ctx;
  int w = 44;
  int h = 8;
  int x = (r.w - w) / 2;
  int y = (r.h - h) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  Rect box = rect_sub(r, x, y, w, h);

  draw_fill(box, ' ', COLOR_DEFAULT, C_BOX_BG, ATTR_NONE);
  draw_box(box, C_BOX_FG, C_BOX_BG, ATTR_NONE);

  Rect inner = rect_inset(box, 2, 1);
  draw_text(inner, 0, 0, "Settings", C_BOX_FG, C_BOX_BG, ATTR_BOLD);

  char line[64];
  snprintf(line, sizeof(line), "Glyphs: %s", g_ascii ? "ASCII" : "Icons");
  uint8_t attr0 = (ui->selected == 0) ? ATTR_REVERSE : ATTR_NONE;
  draw_text(inner, 0, 2, line, C_BOX_FG, C_BOX_BG, attr0);
  ui->row_y[0] = inner.y + 2;

  snprintf(line, sizeof(line), "Colour scheme: %s", palette_name(g_palette));
  uint8_t attr1 = (ui->selected == 1) ? ATTR_REVERSE : ATTR_NONE;
  draw_text(inner, 0, 3, line, C_BOX_FG, C_BOX_BG, attr1);
  ui->row_y[1] = inner.y + 3;

  if (ui->message[0] != '\0') {
    draw_text(inner, 0, 5, ui->message, C_BOX_FG, C_BOX_BG, ATTR_NONE);
  }

  app_draw_bottom_hint(r, "↑/↓ + Enter, or click to select  ·  Esc close");
}

static Cmd_t settings_handle(void *ctx, const Event_t *ev) {
  Settings_ui_t *ui = (Settings_ui_t *)ctx;

  /* A click only moves the highlight, the same as an arrow key — it does not
   * choose the option, so a stray click cannot be mistaken for a confirmed
   * choice. Enter is the one way to act on whatever is selected. */
  if (ev->type == EV_MOUSE && ev->mouse.kind == MOUSE_PRESS && ev->mouse.button == 0) {
    for (int k = 0; k < SETTINGS_ITEM_COUNT; k++) {
      if (ev->mouse.row == ui->row_y[k]) {
        ui->selected = k;
        break;
      }
    }
    return CMD_STAY;
  }
  if (ev->type != EV_KEY) {
    return CMD_STAY;
  }
  if (ev->key.name == KEY_ESCAPE) {
    return (Cmd_t){CMD_POP, NULL};
  }
  if (ev->key.name == KEY_UP) {
    ui->selected = (ui->selected - 1 + SETTINGS_ITEM_COUNT) % SETTINGS_ITEM_COUNT;
    return CMD_STAY;
  }
  if (ev->key.name == KEY_DOWN) {
    ui->selected = (ui->selected + 1) % SETTINGS_ITEM_COUNT;
    return CMD_STAY;
  }
  if (ev->key.name == KEY_ENTER) {
    activate(ui, ui->selected);
    return CMD_STAY;
  }
  return CMD_STAY;
}

Screen *settings_screen(void) {
  static Screen screen;
  screen.on_enter = settings_on_enter;
  screen.on_exit = NULL;
  screen.handle = settings_handle;
  screen.render = settings_render;
  screen.ctx = &g_settings_ui;
  screen.opaque = 0;
  return &screen;
}
