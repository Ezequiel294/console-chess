#include "app/mainmenu.h"

#include "app/game.h"
#include "app/help.h"
#include "app/save.h"
#include "app/savedgames.h"
#include "app/settings.h"
#include "core/history.h"
#include "core/position.h"
#include "ui/render.h"
#include "version.h"

#include <stdio.h>
#include <string.h>

#define C_LABEL 246
#define C_TITLE 250

#define MENU_ITEM_COUNT 5

typedef struct {
  const char *label;
} Menu_entry_t;

typedef struct {
  GameState *state;
  int selected;
  /* Where each entry was last drawn, for hit-testing a click — the row is
   * shared width-wise per line since each is centred independently. */
  int row_y[MENU_ITEM_COUNT];
  int row_x0[MENU_ITEM_COUNT], row_x1[MENU_ITEM_COUNT];
  char message[96];
} MainMenu_t;

static MainMenu_t g_mainmenu;

static const Menu_entry_t ENTRIES[MENU_ITEM_COUNT] = {
    {"New Game"}, {"Load Game"}, {"How to Play"}, {"Settings"}, {"Quit"},
};

static void free_state_lists(GameState *s) {
  free_captures(s->p_captures_white_head);
  free_captures(s->p_captures_black_head);
  free_history(s->p_history_head);
  free_hash_history(s->p_hash_history_head);
  free_history(s->p_redo_head);
}

static void start_fresh_game(MainMenu_t *m) {
  free_state_lists(m->state);
  *m->state = (GameState){0};
  position_init(&m->state->position);
  m->state->start_position = m->state->position;
  push_hash(&m->state->p_hash_history_head, m->state->position.hash);
}

static Cmd_t on_game_loaded(void *ctx, GameState loaded) {
  MainMenu_t *m = (MainMenu_t *)ctx;
  free_state_lists(m->state);
  *m->state = loaded;
  return (Cmd_t){CMD_PUSH, game_screen(m->state)};
}

static Cmd_t activate(MainMenu_t *m, int k) {
  switch (k) {
  case 0:
    start_fresh_game(m);
    return (Cmd_t){CMD_PUSH, game_screen(m->state)};
  case 1:
    return (Cmd_t){CMD_PUSH, savedgames_screen(on_game_loaded, m)};
  case 2:
    return (Cmd_t){CMD_PUSH, help_screen()};
  case 3:
    return (Cmd_t){CMD_PUSH, settings_screen()};
  case 4:
    return (Cmd_t){CMD_QUIT, NULL};
  default:
    return CMD_STAY;
  }
}

static void mainmenu_render(void *ctx, Rect r) {
  MainMenu_t *m = (MainMenu_t *)ctx;

  draw_fill(r, ' ', COLOR_DEFAULT, COLOR_DEFAULT, ATTR_NONE);

  char title[80];
  snprintf(title, sizeof(title), "Console Chess %s", chess_version());
  int tx = (r.w - (int)strlen(title)) / 2;
  if (tx < 0) tx = 0;
  draw_text(r, tx, r.h / 3, title, C_TITLE, COLOR_DEFAULT, ATTR_BOLD);

  int top = r.h / 3 + 2;
  for (int i = 0; i < MENU_ITEM_COUNT; i++) {
    const char *line = ENTRIES[i].label;
    int x = (r.w - (int)strlen(line)) / 2;
    if (x < 0) x = 0;
    uint8_t attr = (i == m->selected) ? ATTR_REVERSE : ATTR_NONE;
    draw_text(r, x, top + i, line, COLOR_DEFAULT, COLOR_DEFAULT, attr);
    m->row_y[i] = top + i;
    m->row_x0[i] = x;
    m->row_x1[i] = x + (int)strlen(line);
  }

  if (m->message[0] != '\0') {
    int mx = (r.w - (int)strlen(m->message)) / 2;
    if (mx < 0) mx = 0;
    draw_text(r, mx, top + MENU_ITEM_COUNT + 2, m->message, C_LABEL, COLOR_DEFAULT, ATTR_NONE);
  }

  app_draw_bottom_hint(r, "↑/↓ + Enter, or click to select");
}

static Cmd_t mainmenu_handle(void *ctx, const Event_t *ev) {
  MainMenu_t *m = (MainMenu_t *)ctx;

  /* A click only moves the highlight, the same as an arrow key — it does not
   * choose the option, so a stray click cannot be mistaken for a confirmed
   * choice. Enter is the one way to act on whatever is selected. */
  if (ev->type == EV_MOUSE && ev->mouse.kind == MOUSE_PRESS && ev->mouse.button == 0) {
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
      if (ev->mouse.row == m->row_y[i] && ev->mouse.col >= m->row_x0[i] &&
          ev->mouse.col < m->row_x1[i]) {
        m->selected = i;
        break;
      }
    }
    return CMD_STAY;
  }
  if (ev->type != EV_KEY) {
    return CMD_STAY;
  }

  switch (ev->key.name) {
  case KEY_UP:
    m->selected = (m->selected - 1 + MENU_ITEM_COUNT) % MENU_ITEM_COUNT;
    return CMD_STAY;
  case KEY_DOWN:
    m->selected = (m->selected + 1) % MENU_ITEM_COUNT;
    return CMD_STAY;
  case KEY_ENTER:
    m->message[0] = '\0';
    return activate(m, m->selected);
  default:
    return CMD_STAY;
  }
}

Screen *mainmenu_screen(GameState *state) {
  static Screen screen;

  g_mainmenu.state = state;
  screen.on_enter = NULL;
  screen.on_exit = NULL;
  screen.handle = mainmenu_handle;
  screen.render = mainmenu_render;
  screen.ctx = &g_mainmenu;
  screen.opaque = 1;
  return &screen;
}
